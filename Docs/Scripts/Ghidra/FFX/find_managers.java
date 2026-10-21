// @desc Find game subsystem functions by searching string refs for key terms
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.address.*;
import ghidra.program.model.data.*;
import java.io.*;
import java.util.*;

public class find_managers extends GhidraScript {
    private File guardedOutput(File output) throws Exception {
        String root = System.getenv("FFX_EVIDENCE_ROOT");
        if (root == null || root.trim().isEmpty()) throw new IllegalStateException("Set FFX_EVIDENCE_ROOT or run the guarded Ghidra wrapper.");
        File canonical = output.getCanonicalFile();
        for (File current = canonical; current != null; current = current.getParentFile())
            if (new File(current, ".git").exists() || (new File(current, "HEAD").isFile() && new File(current, "objects").isDirectory())) throw new IllegalArgumentException("Refusing output inside Git repository: " + canonical);
        return canonical;
    }
    @Override
    public void run() throws Exception {
        currentProgram.setTemporary(true);

        File outputFile = guardedOutput(new File(new File(System.getenv("FFX_EVIDENCE_ROOT"), "ghidra-output/FFX"), "managers_report.txt"));
        if (!outputFile.getParentFile().mkdirs() && !outputFile.getParentFile().isDirectory()) throw new Exception("Cannot create external output directory");
        PrintWriter out = new PrintWriter(new FileWriter(outputFile));

        try {
            Listing listing = currentProgram.getListing();
            ReferenceManager refMgr = currentProgram.getReferenceManager();

            // Categories and their search terms
            String[][] categories = {
                {"TEXT_UI", "text", "string", "font", "message", "dialog", "dialogue",
                    "caption", "label", "display", "show", "print", "render_text",
                    "draw_text", "set_text", "get_text", "locali"},
                {"MENU_SYSTEM", "menu", "cursor", "select", "window", "widget",
                    "screen", "panel", "button", "list", "scroll", "focus",
                    "navigate", "iggy", "swf", "flash", "ui_"},
                {"BATTLE", "battle", "enemy", "attack", "damage", "defend",
                    "command", "turn", "ctb", "atb", "overdrive", "summon",
                    "aeon", "target", "action", "combat", "encounter"},
                {"STATS_CHAR", "hp", "mp", "strength", "defense", "magic",
                    "agility", "luck", "evasion", "accuracy", "level", "exp",
                    "ability", "sphere", "grid", "node", "skill"},
                {"ITEMS_EQUIP", "item", "equip", "weapon", "armor", "accessory",
                    "inventory", "potion", "ether", "elixir", "use_item",
                    "steal", "mix", "customize"},
                {"NAVIGATION", "map", "field", "camera", "position", "move",
                    "walk", "npc", "event", "trigger", "zone", "area",
                    "entrance", "exit", "warp", "teleport", "save_point"},
                {"AUDIO", "sound", "music", "voice", "bgm", "sfx", "fmod",
                    "audio", "volume", "play_sound", "stop_sound"},
                {"SYSTEM", "init", "manager", "system", "engine", "update",
                    "tick", "frame", "render", "draw", "load", "save",
                    "config", "setting", "main_loop", "game_state"},
                {"MINIGAME", "blitz", "blitzball", "chocobo", "race",
                    "score", "goal", "team", "player"}
            };

            out.println("=== FFX.exe Game Subsystem Discovery ===");
            out.println("Finding functions by string reference analysis...");
            out.println();

            // Track unique functions per category to avoid spam
            Map<String, Set<String>> categoryFuncs = new LinkedHashMap<>();
            for (String[] cat : categories) {
                categoryFuncs.put(cat[0], new LinkedHashSet<String>());
            }

            int totalStringsChecked = 0;
            int totalHits = 0;

            // Iterate all defined data looking for strings
            DataIterator dataIter = listing.getDefinedData(true);
            while (dataIter.hasNext()) {
                if (monitor.isCancelled()) break;
                Data data = dataIter.next();
                DataType dt = data.getDataType();
                String dtName = dt.getName().toLowerCase();

                if (!(dtName.contains("string") || dtName.contains("unicode") ||
                      dtName.contains("char[") ||
                      dt instanceof StringDataType || dt instanceof UnicodeDataType ||
                      dt instanceof TerminatedStringDataType ||
                      dt instanceof TerminatedUnicodeDataType)) {
                    continue;
                }

                Object value = data.getValue();
                if (value == null) continue;
                String strVal = value.toString().trim();
                if (strVal.length() < 2 || strVal.length() > 500) continue;

                totalStringsChecked++;
                String lower = strVal.toLowerCase();

                // Check each category
                for (String[] cat : categories) {
                    String catName = cat[0];
                    for (int k = 1; k < cat.length; k++) {
                        if (lower.contains(cat[k])) {
                            // Found a match — find which functions reference this string
                            ReferenceIterator refs =
                                refMgr.getReferencesTo(data.getAddress());
                            while (refs.hasNext()) {
                                if (monitor.isCancelled()) break;
                                Reference ref = refs.next();
                                Address fromAddr = ref.getFromAddress();
                                Function func =
                                    listing.getFunctionContaining(fromAddr);
                                if (func != null) {
                                    String entry = "0x" +
                                        func.getEntryPoint().toString() +
                                        " " + func.getName() +
                                        " (refs string: \"" + strVal + "\"" +
                                        " at 0x" + data.getAddress().toString() +
                                        ", keyword: " + cat[k] + ")";
                                    categoryFuncs.get(catName).add(entry);
                                    totalHits++;
                                }
                            }
                            break; // one keyword match per category per string
                        }
                    }
                }
            }

            // Output results by category
            for (String[] cat : categories) {
                if (monitor.isCancelled()) break;
                String catName = cat[0];
                Set<String> funcs = categoryFuncs.get(catName);
                out.println("=== " + catName + " (" + funcs.size() + " functions) ===");
                out.println();
                for (String entry : funcs) {
                    out.println("  " + entry);
                }
                out.println();
            }

            out.println("--- SUMMARY ---");
            out.println("Strings checked: " + totalStringsChecked);
            out.println("Total hits: " + totalHits);
            for (String[] cat : categories) {
                String catName = cat[0];
                out.println("  " + catName + ": " + categoryFuncs.get(catName).size());
            }

            // Console summary
            println("=== Manager Discovery Complete ===");
            println("Strings checked: " + totalStringsChecked);
            for (String[] cat : categories) {
                String catName = cat[0];
                int count = categoryFuncs.get(catName).size();
                if (count > 0) {
                    println("  " + catName + ": " + count + " functions");
                }
            }
            println("Output: " + outputFile);

        } finally {
            out.close();
        }
    }
}
