// @desc Find all callers of iggy_w32.dll imports in FFX.exe
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.address.*;
import java.io.*;
import java.util.*;

public class find_iggy_xrefs extends GhidraScript {
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

        File outputFile = guardedOutput(new File(new File(System.getenv("FFX_EVIDENCE_ROOT"), "ghidra-output/FFX"), "iggy_xrefs.txt"));
        if (!outputFile.getParentFile().mkdirs() && !outputFile.getParentFile().isDirectory()) throw new Exception("Cannot create external output directory");
        PrintWriter out = new PrintWriter(new FileWriter(outputFile));

        try {
            SymbolTable symTable = currentProgram.getSymbolTable();
            ReferenceManager refMgr = currentProgram.getReferenceManager();
            Listing listing = currentProgram.getListing();

            out.println("=== FFX.exe Iggy Import Cross-References ===");
            out.println();

            int totalXrefs = 0;
            int iggyFuncCount = 0;

            // Find all external symbols from iggy_w32.dll
            SymbolIterator symbols = symTable.getExternalSymbols();
            while (symbols.hasNext()) {
                if (monitor.isCancelled()) break;
                Symbol sym = symbols.next();

                // Check if this external symbol is from iggy_w32.dll
                ExternalLocation extLoc = sym.getProgram()
                    .getExternalManager()
                    .getExternalLocation(sym);
                if (extLoc == null) continue;

                String libName = extLoc.getLibraryName();
                if (libName == null || !libName.toLowerCase().contains("iggy")) continue;

                iggyFuncCount++;
                String funcName = sym.getName();
                out.println("--- " + funcName + " (from " + libName + ") ---");

                // Find all references to this symbol
                ReferenceIterator refs = refMgr.getReferencesTo(sym.getAddress());
                int refCount = 0;
                while (refs.hasNext()) {
                    if (monitor.isCancelled()) break;
                    Reference ref = refs.next();
                    Address fromAddr = ref.getFromAddress();
                    refCount++;
                    totalXrefs++;

                    // Find the containing function
                    Function containingFunc = listing.getFunctionContaining(fromAddr);
                    String funcInfo = "(no function)";
                    if (containingFunc != null) {
                        funcInfo = containingFunc.getName() + " @ 0x" +
                            containingFunc.getEntryPoint().toString();
                    }

                    out.println("  Called from: 0x" + fromAddr.toString() +
                        " in " + funcInfo +
                        " [" + ref.getReferenceType() + "]");
                }

                if (refCount == 0) {
                    out.println("  (no references found — may use indirect call)");
                }
                out.println();
            }

            // Also search for thunk functions that forward to iggy imports
            out.println("--- THUNK FUNCTIONS (iggy wrappers) ---");
            out.println();

            FunctionIterator funcIter = listing.getFunctions(true);
            while (funcIter.hasNext()) {
                if (monitor.isCancelled()) break;
                Function func = funcIter.next();
                if (func.isThunk()) {
                    Function thunkedFunc = func.getThunkedFunction(true);
                    if (thunkedFunc != null && thunkedFunc.isExternal()) {
                        ExternalLocation ext = thunkedFunc.getExternalLocation();
                        if (ext != null) {
                            String lib = ext.getLibraryName();
                            if (lib != null && lib.toLowerCase().contains("iggy")) {
                                out.println("Thunk: " + func.getName() + " @ 0x" +
                                    func.getEntryPoint().toString() +
                                    " -> " + thunkedFunc.getName());

                                // Find callers of the thunk
                                ReferenceIterator thunkRefs =
                                    refMgr.getReferencesTo(func.getEntryPoint());
                                while (thunkRefs.hasNext()) {
                                    if (monitor.isCancelled()) break;
                                    Reference ref = thunkRefs.next();
                                    Address fromAddr = ref.getFromAddress();
                                    Function caller = listing.getFunctionContaining(fromAddr);
                                    String callerInfo = caller != null ?
                                        caller.getName() + " @ 0x" +
                                        caller.getEntryPoint().toString() :
                                        "(no function)";
                                    out.println("  Called from: 0x" + fromAddr.toString() +
                                        " in " + callerInfo);
                                    totalXrefs++;
                                }
                                out.println();
                            }
                        }
                    }
                }
            }

            out.println("--- SUMMARY ---");
            out.println("Iggy imports found: " + iggyFuncCount);
            out.println("Total cross-references: " + totalXrefs);

            println("=== Iggy Xref Complete ===");
            println("Iggy imports: " + iggyFuncCount);
            println("Total xrefs:  " + totalXrefs);
            println("Output: " + outputFile);

        } finally {
            out.close();
        }
    }
}
