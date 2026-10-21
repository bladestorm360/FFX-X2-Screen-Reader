// @desc Decompile ALL functions in FFX.exe — full pseudocode dump by address range
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import java.io.*;
import java.util.*;

public class decompile_all extends GhidraScript {
    private File guardedOutput(File output) throws Exception {
        String root = System.getenv("FFX_EVIDENCE_ROOT");
        if (root == null || root.trim().isEmpty()) throw new IllegalStateException("Set FFX_EVIDENCE_ROOT or run the guarded Ghidra wrapper.");
        File canonical = output.getCanonicalFile();
        for (File current = canonical; current != null; current = current.getParentFile())
            if (new File(current, ".git").exists() || (new File(current, "HEAD").isFile() && new File(current, "objects").isDirectory()))
                throw new IllegalArgumentException("Refusing output inside Git repository: " + canonical);
        return canonical;
    }
    @Override
    public void run() throws Exception {
        currentProgram.setTemporary(true);

        File dir = guardedOutput(new File(System.getenv("FFX_EVIDENCE_ROOT"), "ghidra-output/FFX/decompile"));
        String outDir = dir.getAbsolutePath();

        // Create output directory
        if (!dir.mkdirs() && !dir.isDirectory()) throw new Exception("Cannot create external output directory: " + dir);

        // Set up decompiler
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);

        // Set a generous timeout per function (30 seconds)
        decomp.setOptions(new DecompileOptions());

        Listing listing = currentProgram.getListing();

        // Count functions first
        int totalFuncs = 0;
        FunctionIterator counter = listing.getFunctions(true);
        while (counter.hasNext()) {
            counter.next();
            totalFuncs++;
        }
        println("Total functions to decompile: " + totalFuncs);

        // Define output files by address range
        // Each range gets its own file to keep sizes manageable
        String[][] ranges = {
            {"0x00400000", "0x00640000", "layer1_phyreengine.txt"},
            {"0x00640000", "0x00900000", "layer2_game_logic.txt"},
            {"0x00900000", "0x00a20000", "layer2b_game_extended.txt"},
            {"0x00a20000", "0x00a50000", "layer3_iggy_interface.txt"},
            {"0x00a50000", "0x00b10000", "layer4_menu_font_native.txt"},
            {"0x00b10000", "0x00d00000", "layer5_data_misc.txt"},
        };

        // Open all output files
        Map<String, PrintWriter> writers = new LinkedHashMap<>();
        for (String[] range : ranges) {
            String path = outDir + "/" + range[2];
            writers.put(range[2], new PrintWriter(new FileWriter(path)));
            writers.get(range[2]).println("=== " + range[2] + " ===");
            writers.get(range[2]).println("Address range: " + range[0] + " - " + range[1]);
            writers.get(range[2]).println();
        }

        // Catch-all for addresses outside defined ranges
        PrintWriter miscWriter = new PrintWriter(new FileWriter(outDir + "/other.txt"));
        miscWriter.println("=== Functions outside defined ranges ===");
        miscWriter.println();

        int processed = 0;
        int succeeded = 0;
        int failed = 0;

        FunctionIterator funcIter = listing.getFunctions(true);
        while (funcIter.hasNext()) {
            if (monitor.isCancelled()) {
                println("Cancelled at function " + processed + "/" + totalFuncs);
                break;
            }

            Function func = funcIter.next();
            processed++;

            // Progress to console every 500 functions
            if (processed % 500 == 0) {
                println("Progress: " + processed + "/" + totalFuncs +
                    " (" + succeeded + " ok, " + failed + " failed)");
            }

            // Determine which output file based on address
            long addr = func.getEntryPoint().getOffset();
            PrintWriter out = miscWriter;
            for (String[] range : ranges) {
                long lo = Long.parseLong(range[0].substring(2), 16);
                long hi = Long.parseLong(range[1].substring(2), 16);
                if (addr >= lo && addr < hi) {
                    out = writers.get(range[2]);
                    break;
                }
            }

            // Decompile
            DecompileResults results = decomp.decompileFunction(func, 30, monitor);

            String header = "// ========================================\n" +
                "// Function: " + func.getName() + "\n" +
                "// Address:  0x" + func.getEntryPoint().toString() + "\n" +
                "// Size:     " + func.getBody().getNumAddresses() + " bytes\n" +
                "// Calling:  " + func.getCallingConventionName() + "\n" +
                "// ========================================";

            out.println(header);

            if (results != null && results.getDecompiledFunction() != null) {
                String code = results.getDecompiledFunction().getC();
                if (code != null && !code.isEmpty()) {
                    out.println(code);
                    succeeded++;
                } else {
                    out.println("// (decompiler returned empty output)");
                    failed++;
                }
            } else {
                String errMsg = (results != null) ? results.getErrorMessage() : "null result";
                out.println("// (decompile failed: " + errMsg + ")");
                failed++;
            }
            out.println();
        }

        // Close all writers
        for (PrintWriter w : writers.values()) {
            w.close();
        }
        miscWriter.close();
        decomp.dispose();

        // Summary
        println("=== Decompile Complete ===");
        println("Processed: " + processed + "/" + totalFuncs);
        println("Succeeded: " + succeeded);
        println("Failed:    " + failed);
        println("Output:    " + outDir);
    }
}
