// @desc Export RTTI class names, vtables, Function ID matches, and improved signatures
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.address.*;
import java.io.*;
import java.util.*;

public class export_rtti extends GhidraScript {
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

        String progName = currentProgram.getName().replace(".exe", "");
        File outputDirectory = guardedOutput(new File(new File(System.getenv("FFX_EVIDENCE_ROOT"), "ghidra-output"), progName));
        if (!outputDirectory.mkdirs() && !outputDirectory.isDirectory()) throw new Exception("Cannot create external output directory: " + outputDirectory);
        String outDir = outputDirectory.getAbsolutePath() + File.separator;

        String outPath = outDir + "rtti_classes.txt";
        PrintWriter out = new PrintWriter(new FileWriter(outPath));

        try {
            println("=== RTTI + Analysis Export for " + currentProgram.getName() + " ===");
            out.println("=== RTTI + Analysis Export for " + currentProgram.getName() + " ===");
            out.println("Generated: " + new Date());
            out.println();

            SymbolTable symTab = currentProgram.getSymbolTable();
            Listing listing = currentProgram.getListing();

            // ── Section 1: RTTI Class Names (vtable → class) ──
            out.println("=== RTTI CLASSES (vtable → class name) ===");
            out.println();

            int classCount = 0;
            Map<String, List<String>> namespaceMap = new TreeMap<>();

            SymbolIterator allSyms = symTab.getAllSymbols(true);
            while (allSyms.hasNext()) {
                if (monitor.isCancelled()) break;
                Symbol sym = allSyms.next();
                String name = sym.getName();
                String fullName = sym.getName(true); // includes namespace

                // RTTI vtables: "vftable" label
                if (name.contains("vftable") || name.contains("vbtable")) {
                    Address addr = sym.getAddress();
                    out.println("[VTABLE] " + String.format("0x%08x", addr.getOffset())
                        + " " + fullName);
                    classCount++;

                    // Extract namespace (class name)
                    Namespace ns = sym.getParentNamespace();
                    if (ns != null && !ns.isGlobal()) {
                        String className = ns.getName(true);
                        if (!namespaceMap.containsKey(className)) {
                            namespaceMap.put(className, new ArrayList<>());
                        }
                        namespaceMap.get(className).add(
                            String.format("0x%08x", addr.getOffset()) + " " + name);
                    }
                }

                // RTTI Type Descriptors
                if (name.contains("RTTI_Type_Descriptor") ||
                    name.contains("RTTI_Complete_Object_Locator") ||
                    name.contains("RTTI_Class_Hierarchy")) {
                    Address addr = sym.getAddress();
                    out.println("[RTTI]   " + String.format("0x%08x", addr.getOffset())
                        + " " + fullName);
                }
            }

            out.println();
            out.println("Total vtables found: " + classCount);
            println("Total vtables found: " + classCount);

            // ── Section 2: Class Summary (grouped by namespace) ──
            out.println();
            out.println("=== CLASS SUMMARY (by namespace) ===");
            out.println();

            for (Map.Entry<String, List<String>> entry : namespaceMap.entrySet()) {
                out.println("CLASS: " + entry.getKey());
                for (String vtable : entry.getValue()) {
                    out.println("  " + vtable);
                }
            }
            println("Unique classes with vtables: " + namespaceMap.size());

            // ── Section 3: Named Functions (non-FUN_ names) ──
            out.println();
            out.println("=== NAMED FUNCTIONS (non-default names from FID/RTTI/user) ===");
            out.println();

            int namedCount = 0;
            FunctionIterator funcIt = listing.getFunctions(true);
            while (funcIt.hasNext()) {
                if (monitor.isCancelled()) break;
                Function func = funcIt.next();
                String fname = func.getName();
                String fullFName = func.getName(true);

                // Skip default FUN_ names and thunks to default names
                if (fname.startsWith("FUN_") || fname.startsWith("thunk_FUN_")) continue;

                // This function has a meaningful name (from RTTI, FID, imports, etc.)
                Address faddr = func.getEntryPoint();
                String sig = func.getSignature().getPrototypeString();
                out.println(String.format("0x%08x  %s", faddr.getOffset(), fullFName));
                out.println("  Signature: " + sig);

                Namespace fns = func.getParentNamespace();
                if (fns != null && !fns.isGlobal()) {
                    out.println("  Namespace: " + fns.getName(true));
                }
                out.println();
                namedCount++;
            }
            println("Named functions: " + namedCount);

            // ── Section 4: Imported Functions ──
            out.println();
            out.println("=== IMPORTED FUNCTIONS ===");
            out.println();

            int importCount = 0;
            SymbolIterator extSyms = symTab.getExternalSymbols();
            while (extSyms.hasNext()) {
                if (monitor.isCancelled()) break;
                Symbol ext = extSyms.next();
                if (ext.getSymbolType() == SymbolType.FUNCTION) {
                    out.println(ext.getName(true));
                    importCount++;
                }
            }
            println("Imported functions: " + importCount);

            // ── Section 5: All Namespaces (class hierarchy) ──
            out.println();
            out.println("=== ALL NAMESPACES ===");
            out.println();

            int nsCount = 0;
            Iterator<GhidraClass> classIt = symTab.getClassNamespaces();
            while (classIt.hasNext()) {
                if (monitor.isCancelled()) break;
                GhidraClass gc = classIt.next();
                out.println("  " + gc.getName(true));
                nsCount++;
            }
            println("Class namespaces: " + nsCount);

            // ── Section 6: Statistics ──
            out.println();
            out.println("=== STATISTICS ===");
            int totalFuncs = 0;
            FunctionIterator countIt = listing.getFunctions(true);
            while (countIt.hasNext()) {
                countIt.next();
                totalFuncs++;
            }
            out.println("Total functions: " + totalFuncs);
            out.println("Named functions: " + namedCount);
            out.println("Vtables found: " + classCount);
            out.println("Unique classes: " + namespaceMap.size());
            out.println("Class namespaces: " + nsCount);
            out.println("Imported functions: " + importCount);

            println("=== Export Complete === Output: " + outPath);
        } finally {
            out.close();
        }
    }
}
