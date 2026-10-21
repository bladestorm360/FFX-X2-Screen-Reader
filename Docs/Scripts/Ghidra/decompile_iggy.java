// @desc Decompile iggy_w32.dll — full pseudocode dump + RTTI export
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.address.*;
import java.io.*;
import java.util.*;

public class decompile_iggy extends GhidraScript {
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

        File outputDirectory = guardedOutput(new File(System.getenv("FFX_EVIDENCE_ROOT"), "ghidra-output/iggy"));
        if (!outputDirectory.mkdirs() && !outputDirectory.isDirectory()) throw new Exception("Cannot create external output directory: " + outputDirectory);
        String outDir = outputDirectory.getAbsolutePath();

        // ── Part 1: RTTI Export ──
        println("=== RTTI Export for " + currentProgram.getName() + " ===");
        PrintWriter rttiOut = new PrintWriter(new FileWriter(outDir + "/rtti_classes.txt"));
        try {
            rttiOut.println("=== RTTI Export for " + currentProgram.getName() + " ===");
            rttiOut.println("Generated: " + new Date());
            rttiOut.println();

            SymbolTable symTab = currentProgram.getSymbolTable();
            Listing listing = currentProgram.getListing();

            // Find vtables
            rttiOut.println("=== VTABLES ===");
            int vtableCount = 0;
            Map<String, List<String>> classes = new TreeMap<>();

            SymbolIterator allSyms = symTab.getAllSymbols(true);
            while (allSyms.hasNext()) {
                if (monitor.isCancelled()) break;
                Symbol sym = allSyms.next();
                String name = sym.getName();
                String full = sym.getName(true);

                if (name.contains("vftable") || name.contains("vbtable")) {
                    rttiOut.println("[VTABLE] 0x" + String.format("%08x", sym.getAddress().getOffset()) + " " + full);
                    vtableCount++;
                    Namespace ns = sym.getParentNamespace();
                    if (ns != null && !ns.isGlobal()) {
                        String cn = ns.getName(true);
                        classes.computeIfAbsent(cn, k -> new ArrayList<>()).add(
                            String.format("0x%08x %s", sym.getAddress().getOffset(), name));
                    }
                }
                if (name.contains("RTTI_Type_Descriptor") || name.contains("RTTI_Complete") || name.contains("RTTI_Class_Hierarchy")) {
                    rttiOut.println("[RTTI]   0x" + String.format("%08x", sym.getAddress().getOffset()) + " " + full);
                }
            }

            rttiOut.println();
            rttiOut.println("=== CLASSES ===");
            for (Map.Entry<String, List<String>> e : classes.entrySet()) {
                rttiOut.println("CLASS: " + e.getKey());
                for (String v : e.getValue()) rttiOut.println("  " + v);
            }

            // Named functions
            rttiOut.println();
            rttiOut.println("=== NAMED FUNCTIONS ===");
            int namedCount = 0;
            FunctionIterator funcIt = listing.getFunctions(true);
            while (funcIt.hasNext()) {
                if (monitor.isCancelled()) break;
                Function f = funcIt.next();
                if (!f.getName().startsWith("FUN_") && !f.getName().startsWith("thunk_FUN_")) {
                    rttiOut.println(String.format("0x%08x  %s  %s", f.getEntryPoint().getOffset(),
                        f.getName(true), f.getSignature().getPrototypeString()));
                    namedCount++;
                }
            }

            println("Vtables: " + vtableCount + ", Classes: " + classes.size() + ", Named: " + namedCount);
        } finally {
            rttiOut.close();
        }

        // ── Part 2: Full Decompile ──
        println("=== Starting full decompile ===");
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        decomp.setOptions(new DecompileOptions());

        Listing listing = currentProgram.getListing();
        int total = 0, ok = 0, fail = 0;

        // Count functions
        FunctionIterator counter = listing.getFunctions(true);
        while (counter.hasNext()) { counter.next(); total++; }
        println("Total functions: " + total);

        // Decompile all to single file
        PrintWriter out = new PrintWriter(new FileWriter(outDir + "/decompile_all.txt"));
        try {
            out.println("// iggy_w32.dll full decompile");
            out.println("// Functions: " + total);
            out.println("// Generated: " + new Date());
            out.println();

            int count = 0;
            FunctionIterator fi = listing.getFunctions(true);
            while (fi.hasNext()) {
                if (monitor.isCancelled()) break;
                Function func = fi.next();
                count++;

                if (count % 500 == 0) {
                    println("Progress: " + count + "/" + total + " (" + ok + " ok, " + fail + " failed)");
                }

                try {
                    DecompileResults res = decomp.decompileFunction(func, 30, monitor);
                    if (res.decompileCompleted()) {
                        String code = res.getDecompiledFunction().getC();
                        out.println("// ========================================");
                        out.println("// Function: " + func.getName(true));
                        out.println("// Address:  0x" + String.format("%08x", func.getEntryPoint().getOffset()));
                        out.println("// Size:     " + func.getBody().getNumAddresses() + " bytes");
                        out.println("// ========================================");
                        out.println();
                        out.println(code);
                        out.println();
                        ok++;
                    } else {
                        fail++;
                    }
                } catch (Exception e) {
                    fail++;
                }
            }

            println("=== Decompile Complete ===");
            println("Processed: " + count + "/" + total);
            println("Succeeded: " + ok);
            println("Failed:    " + fail);
        } finally {
            out.close();
            decomp.dispose();
        }
    }
}
