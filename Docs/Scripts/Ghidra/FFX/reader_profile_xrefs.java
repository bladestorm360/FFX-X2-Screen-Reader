// @desc Export instruction references for unresolved native reader addresses, read-only
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.symbol.Reference;
import java.io.*;
import java.nio.file.Files;
import java.text.SimpleDateFormat;
import java.util.Date;

public class reader_profile_xrefs extends GhidraScript {
    private File guardedOutput(File output) throws Exception {
        String root = System.getenv("FFX_EVIDENCE_ROOT");
        if (root == null || root.trim().isEmpty()) throw new IllegalStateException("Set FFX_EVIDENCE_ROOT or run the guarded Ghidra wrapper.");
        File canonical = output.getCanonicalFile();
        for (File current = canonical; current != null; current = current.getParentFile())
            if (new File(current, ".git").exists() || (new File(current, "HEAD").isFile() && new File(current, "objects").isDirectory())) throw new IllegalArgumentException("Refusing output inside Git repository: " + canonical);
        return canonical;
    }
    @Override public void run() throws Exception {
        currentProgram.setTemporary(true);
        File file = guardedOutput(new File(getScriptArgs()[1], "reader_xrefs_" +
            new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date()) + ".tsv"));
        long base = currentProgram.getImageBase().getOffset();
        try (PrintWriter output = new PrintWriter(file)) {
            for (String value : Files.readAllLines(new File(getScriptArgs()[0]).toPath())) {
                long target = Long.decode(value);
                for (Reference ref : currentProgram.getReferenceManager().getReferencesTo(toAddr(base + target))) {
                    Instruction anchor = currentProgram.getListing().getInstructionAt(ref.getFromAddress());
                    if (anchor == null) continue;
                    Instruction start = anchor, end = anchor;
                    for (int j = 0; j < 12; j++) {
                        Instruction prior = start.getPrevious();
                        if (prior == null || !prior.getMaxAddress().add(1).equals(start.getAddress())) break;
                        start = prior;
                    }
                    for (int j = 0; j < 6; j++) {
                        Instruction next = end.getNext();
                        if (next == null || !end.getMaxAddress().add(1).equals(next.getAddress())) break;
                        end = next;
                    }
                    int size = (int)end.getMaxAddress().subtract(start.getAddress()) + 1;
                    byte[] bytes = new byte[size];
                    currentProgram.getMemory().getBytes(start.getAddress(), bytes);
                    StringBuilder hex = new StringBuilder();
                    for (byte b : bytes) hex.append(String.format("%02x", b & 255));
                    output.println(Long.toHexString(target) + "\t" +
                        Long.toHexString(anchor.getAddress().getOffset() - base) + "\t" +
                        Long.toHexString(start.getAddress().getOffset() - base) + "\t" + hex);
                }
            }
        }
        println("Read-only instruction references: " + file.getAbsolutePath());
    }
}
