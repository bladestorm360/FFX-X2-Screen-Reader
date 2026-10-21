// @desc Dump all defined strings in FFX.exe, filtered for game-relevant terms
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.data.*;
import ghidra.program.model.address.*;
import java.io.*;

public class dump_strings extends GhidraScript {
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

        File outputFile = guardedOutput(new File(new File(System.getenv("FFX_EVIDENCE_ROOT"), "ghidra-output/FFX"), "strings_dump.txt"));
        if (!outputFile.getParentFile().mkdirs() && !outputFile.getParentFile().isDirectory()) throw new Exception("Cannot create external output directory");
        PrintWriter out = new PrintWriter(new FileWriter(outputFile));

        try {
            Listing listing = currentProgram.getListing();
            int totalCount = 0;
            int interestingCount = 0;

            // Interesting keywords for game subsystem identification
            String[] keywords = {
                "text", "string", "font", "message", "dialog", "dialogue",
                "menu", "cursor", "select", "window", "iggy", "swf", "flash",
                "battle", "enemy", "attack", "damage", "turn", "command",
                "hp", "mp", "str", "level", "exp", "ability", "magic",
                "sphere", "grid", "node", "path",
                "item", "equip", "weapon", "armor",
                "save", "load", "config", "option",
                "map", "field", "camera", "position", "npc",
                "blitz", "chocobo",
                "party", "character", "tidus", "yuna", "auron",
                "new game", "continue", "load game",
                "file", "data", "init", "manager", "system", "render", "draw",
                "error", "debug", "log", "assert"
            };

            out.println("=== FFX.exe String Dump ===");
            out.println("Searching all defined data for string types...");
            out.println();

            // Section 1: All strings (to file only)
            out.println("--- ALL DEFINED STRINGS ---");
            out.println();

            DataIterator dataIter = listing.getDefinedData(true);
            while (dataIter.hasNext()) {
                if (monitor.isCancelled()) break;
                Data data = dataIter.next();
                DataType dt = data.getDataType();
                String dtName = dt.getName().toLowerCase();

                // Check for string-type data
                if (dtName.contains("string") || dtName.contains("unicode") ||
                    dtName.contains("char[") || dtName.contains("char *") ||
                    dt instanceof StringDataType || dt instanceof UnicodeDataType ||
                    dt instanceof TerminatedStringDataType || dt instanceof TerminatedUnicodeDataType) {

                    Object value = data.getValue();
                    if (value != null) {
                        String strVal = value.toString().trim();
                        if (strVal.length() > 1 && strVal.length() < 1000) {
                            totalCount++;
                            String addr = data.getAddress().toString();
                            out.println("[0x" + addr + "] " + strVal);

                            // Check if interesting
                            String lower = strVal.toLowerCase();
                            for (String kw : keywords) {
                                if (lower.contains(kw)) {
                                    interestingCount++;
                                    out.println("  ^^^ INTERESTING (matches: " + kw + ")");
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            out.println();
            out.println("--- SUMMARY ---");
            out.println("Total strings found: " + totalCount);
            out.println("Interesting strings: " + interestingCount);

            // Console summary
            println("=== String Dump Complete ===");
            println("Total strings: " + totalCount);
            println("Interesting:   " + interestingCount);
            println("Output: " + outputFile);

        } finally {
            out.close();
        }
    }
}
