// @desc Auto-label functions by string references + export symbol table CSV
//
// Scans ALL functions in the program. For each function that contains a string
// reference (printf format strings, debug messages, etc.), labels the function
// with a meaningful name derived from that string.
//
// Also exports functions.csv and globals.csv compatible with Fahrenheit's STEP tool.
//
// Output: auto_label_report.txt (summary) + functions.csv + globals.csv

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.address.*;
import ghidra.program.model.data.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.scalar.*;
import java.io.*;
import java.util.*;

public class auto_label_functions extends GhidraScript {
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

        File outputDirectory = guardedOutput(new File(System.getenv("FFX_EVIDENCE_ROOT"), "ghidra-output/FFX"));
        if (!outputDirectory.mkdirs() && !outputDirectory.isDirectory()) throw new Exception("Cannot create external output directory: " + outputDirectory);
        String outDir = outputDirectory.getAbsolutePath() + File.separator;
        PrintWriter report = new PrintWriter(new FileWriter(outDir + "auto_label_report.txt"));
        PrintWriter funcCsv = new PrintWriter(new FileWriter(outDir + "functions.csv"));
        PrintWriter globalCsv = new PrintWriter(new FileWriter(outDir + "globals.csv"));

        try {
            Listing listing = currentProgram.getListing();
            ReferenceManager refMgr = currentProgram.getReferenceManager();
            SymbolTable symTable = currentProgram.getSymbolTable();
            Memory memory = currentProgram.getMemory();

            // CSV headers (STEP format)
            funcCsv.println("Name,Location,Function Signature,Symbol Source,Symbol Type,Function Name,Call Conv,Namespace");
            globalCsv.println("Name,Location,Type,Data Type,Namespace,Source");

            int labeledCount = 0;
            int totalFunctions = 0;
            int stringRefCount = 0;

            // Map: function address -> best string label
            Map<String, String> funcLabels = new LinkedHashMap<>();
            // Map: function address -> all strings found
            Map<String, List<String>> funcStrings = new LinkedHashMap<>();

            // Iterate all functions
            FunctionIterator funcIter = listing.getFunctions(true);
            while (funcIter.hasNext() && !monitor.isCancelled()) {
                Function func = funcIter.next();
                totalFunctions++;

                String funcAddr = func.getEntryPoint().toString();
                String funcName = func.getName();
                List<String> strings = new ArrayList<>();

                // Scan the function body for references to string data
                AddressSetView body = func.getBody();
                InstructionIterator instrIter = listing.getInstructions(body, true);

                while (instrIter.hasNext() && !monitor.isCancelled()) {
                    Instruction instr = instrIter.next();

                    // Check all references from this instruction
                    Reference[] refs = instr.getReferencesFrom();
                    for (Reference ref : refs) {
                        Address toAddr = ref.getToAddress();
                        Data data = listing.getDataAt(toAddr);

                        if (data != null && data.hasStringValue()) {
                            String strVal = data.getDefaultValueRepresentation();
                            if (strVal != null && strVal.length() > 3) {
                                // Clean up the string for use as a label
                                strVal = strVal.replace("\"", "").trim();
                                if (strVal.length() > 3 && !strVal.startsWith("\\")) {
                                    strings.add(strVal);
                                    stringRefCount++;
                                }
                            }
                        }
                    }
                }

                if (!strings.isEmpty()) {
                    funcStrings.put(funcAddr, strings);

                    // Pick the best label: prefer short, descriptive strings
                    // Skip generic strings like single characters or numbers
                    String bestLabel = null;
                    for (String s : strings) {
                        if (s.length() < 4) continue;
                        if (s.startsWith("%")) continue; // skip pure format strings
                        if (s.matches("^\\d+$")) continue; // skip pure numbers

                        // Prefer strings with keywords
                        String lower = s.toLowerCase();
                        boolean hasKeyword = lower.contains("map") || lower.contains("field") ||
                            lower.contains("event") || lower.contains("jump") || lower.contains("zone") ||
                            lower.contains("area") || lower.contains("trigger") || lower.contains("exit") ||
                            lower.contains("door") || lower.contains("warp") || lower.contains("load") ||
                            lower.contains("init") || lower.contains("error") || lower.contains("debug") ||
                            lower.contains("player") || lower.contains("pos") || lower.contains("path") ||
                            lower.contains("link") || lower.contains("save") || lower.contains("battle") ||
                            lower.contains("menu") || lower.contains("tk:") || lower.contains("sg:") ||
                            lower.contains("mpf") || lower.contains("ppp") || lower.contains("jck");

                        if (hasKeyword) {
                            bestLabel = s;
                            break;
                        }
                        if (bestLabel == null) {
                            bestLabel = s;
                        }
                    }

                    if (bestLabel != null) {
                        funcLabels.put(funcAddr, bestLabel);
                        labeledCount++;
                    }
                }

                // Export to functions.csv (ALL functions, not just labeled ones)
                String sig = func.getSignature().getPrototypeString();
                String source = funcName.startsWith("FUN_") ? "ANALYSIS" : "USER_DEFINED";
                String callConv = func.getCallingConventionName();
                if (callConv == null) callConv = "unknown";

                // Escape commas in signature for CSV
                sig = "\"" + sig.replace("\"", "\"\"") + "\"";

                funcCsv.println(funcName + "," + func.getEntryPoint().toString() + "," +
                    sig + "," + source + ",Function," + funcName + "," + callConv + ",Global");
            }

            // Write report
            report.println("=== Auto-Label Report ===");
            report.println("Total functions: " + totalFunctions);
            report.println("Functions with string refs: " + funcStrings.size());
            report.println("Functions labeled: " + labeledCount);
            report.println("Total string references: " + stringRefCount);
            report.println();

            // Write labeled functions sorted by address
            report.println("=== Labeled Functions ===");
            for (Map.Entry<String, String> entry : funcLabels.entrySet()) {
                String addr = entry.getKey();
                String label = entry.getValue();
                List<String> allStrings = funcStrings.get(addr);

                report.println(addr + " | " + label);
                if (allStrings.size() > 1) {
                    for (String s : allStrings) {
                        report.println("    " + s);
                    }
                }
            }

            // Write transition-related functions separately for easy scanning
            report.println();
            report.println("=== TRANSITION-RELATED FUNCTIONS ===");
            String[] transitionKeywords = {"map", "field", "jump", "event", "zone", "area",
                "trigger", "exit", "door", "warp", "load", "transition", "room", "entrance",
                "link", "path", "mpf", "ppp", "tk:", "sg:"};

            for (Map.Entry<String, String> entry : funcLabels.entrySet()) {
                String label = entry.getValue().toLowerCase();
                boolean isRelevant = false;
                for (String kw : transitionKeywords) {
                    if (label.contains(kw)) { isRelevant = true; break; }
                }
                if (isRelevant) {
                    report.println(entry.getKey() + " | " + entry.getValue());
                }
            }

            // Export globals
            SymbolIterator symIter = symTable.getAllSymbols(true);
            int globalCount = 0;
            while (symIter.hasNext() && !monitor.isCancelled()) {
                Symbol sym = symIter.next();
                if (sym.getSymbolType() == SymbolType.LABEL && sym.getSource() == SourceType.USER_DEFINED) {
                    Data data = listing.getDataAt(sym.getAddress());
                    String dataType = (data != null) ? data.getDataType().getName() : "undefined";
                    globalCsv.println(sym.getName() + "," + sym.getAddress().toString() + ",Data Label," +
                        dataType + "," + sym.getParentNamespace().getName() + ",User Defined");
                    globalCount++;
                }
            }

            report.println();
            report.println("=== Globals exported: " + globalCount + " ===");

            println("=== Auto-Label Complete ===");
            println("Labeled " + labeledCount + " of " + totalFunctions + " functions");
            println("Report: " + outDir + "auto_label_report.txt");
            println("CSV: " + outDir + "functions.csv + globals.csv");

        } finally {
            report.close();
            funcCsv.close();
            globalCsv.close();
        }
    }
}
