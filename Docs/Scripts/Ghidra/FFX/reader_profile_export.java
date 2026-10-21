// @desc Read-only export of existing program bytes for native reader address audit
import ghidra.app.script.GhidraScript;
import ghidra.program.model.mem.MemoryBlock;
import java.io.*;
import java.text.SimpleDateFormat;
import java.util.Date;

public class reader_profile_export extends GhidraScript {
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
        File directory = guardedOutput(new File(getScriptArgs()[0],
            "reader_reference_" + new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date())));
        if (!directory.mkdirs() && !directory.isDirectory()) throw new Exception("output directory");
        try (PrintWriter info = new PrintWriter(new File(directory, "program.tsv"));
             PrintWriter blocks = new PrintWriter(new File(directory, "blocks.tsv"))) {
            info.println("name\t" + currentProgram.getName());
            info.println("sha256\t" + currentProgram.getExecutableSHA256());
            info.println("image_base\t" + currentProgram.getImageBase());
            info.println("functions\t" + currentProgram.getFunctionManager().getFunctionCount());
            for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
                if (!block.isInitialized() || block.getSize() > 32 * 1024 * 1024) continue;
                String name = "block_" + block.getStart() + ".bin";
                blocks.println(block.getName() + "\t" + block.getStart() + "\t" +
                    block.getSize() + "\t" + block.isExecute() + "\t" + name);
                try (FileOutputStream output = new FileOutputStream(new File(directory, name))) {
                    byte[] buffer = new byte[65536];
                    for (long offset = 0; offset < block.getSize();) {
                        monitor.checkCancelled();
                        int length = (int)Math.min(buffer.length, block.getSize() - offset);
                        currentProgram.getMemory().getBytes(block.getStart().add(offset), buffer, 0, length);
                        output.write(buffer, 0, length);
                        offset += length;
                    }
                }
            }
        }
        println("Read-only native reference exported: " + directory.getAbsolutePath());
    }
}
