// @desc Read/decompile NPC model/menu wrappers and Items native cursor/text anchors
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.*;
import java.text.SimpleDateFormat;
import java.util.*;

public class native_reader_anchors extends GhidraScript {
    private File guardedOutput(File output) throws Exception {
        String root = System.getenv("FFX_EVIDENCE_ROOT");
        if (root == null || root.trim().isEmpty()) throw new IllegalStateException("Set FFX_EVIDENCE_ROOT or run the guarded Ghidra wrapper.");
        File canonical = output.getCanonicalFile();
        for (File current = canonical; current != null; current = current.getParentFile())
            if (new File(current, ".git").exists() || (new File(current, "HEAD").isFile() && new File(current, "objects").isDirectory())) throw new IllegalArgumentException("Refusing output inside Git repository: " + canonical);
        return canonical;
    }
    private DecompInterface decompiler;
    private PrintWriter output;
    private final Set<Long> seen = new HashSet<Long>();
    private void dump(long va, String label) throws Exception {
        Address address = toAddr(va);
        output.println(label + " VA=" + address + " RVA=" + Long.toHexString(va - currentProgram.getImageBase().getOffset()));
        Function function = getFunctionAt(address);
        if (function == null) { output.println("No existing function; database left unchanged."); return; }
        if (!seen.add(va)) return;
        DecompileResults result = decompiler.decompileFunction(function, 30, monitor);
        if (result.decompileCompleted()) output.println(result.getDecompiledFunction().getC());
        else output.println("Decompile failed: " + result.getErrorMessage());
    }
    private void call(long table, int id, String lead) throws Exception {
        Address row = toAddr(table + (id & 0xFFF) * 16L);
        long target = Integer.toUnsignedLong(currentProgram.getMemory().getInt(row.add(4)));
        output.println("Reference lead: " + lead + " call=" + Integer.toHexString(id) + " table=" + row);
        if (target != 0) dump(target, lead);
    }
    @Override public void run() throws Exception {
        currentProgram.setTemporary(true);
        File directory = guardedOutput(new File(System.getenv("FFX_EVIDENCE_ROOT"), "ghidra-output/FFX/native_reader_anchors"));
        if (!directory.mkdirs() && !directory.isDirectory()) throw new Exception("Cannot create external output directory: " + directory);
        File file = new File(directory, new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date()) + ".txt");
        output = new PrintWriter(new FileWriter(file));
        decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        try {
            output.println("Read-only discovery: reference names require body/call-context verification.");
            call(0x850050L, 0x0001, "loadModel");
            call(0x850050L, 0x0134, "loadModel2");
            call(0x850050L, 0x0064, "displayFieldString");
            call(0x850050L, 0x0225, "setShopPrices");
            call(0x888D88L, 0x401D, "showModularMenu");
            dump(0x008C9110L, "Items window builder");
            dump(0x008C6DA0L, "Items window input state");
            dump(0x008C7720L, "Items window rendering/native text calls");
            dump(0x008B4460L, "TkWindow input mutation");
            dump(0x009093B0L, "Resource load phase; not actor-population completion");
        } finally { output.close(); decompiler.dispose(); }
        println("Evidence: " + file.getAbsolutePath());
    }
}
