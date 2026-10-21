// Offline audit of the user's own shipped proxy; never executes the DLL.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import java.io.File;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.LinkedHashSet;
import ghidra.program.model.listing.Function;
public class shipped_threading_audit extends GhidraScript {
    private File guardedOutput(File output) throws Exception {
        String root = System.getenv("FFX_EVIDENCE_ROOT");
        if (root == null || root.trim().isEmpty()) throw new IllegalStateException("Set FFX_EVIDENCE_ROOT or run the guarded Ghidra wrapper.");
        File canonical = output.getCanonicalFile();
        for (File current = canonical; current != null; current = current.getParentFile())
            if (new File(current, ".git").exists() || (new File(current, "HEAD").isFile() && new File(current, "objects").isDirectory())) throw new IllegalArgumentException("Refusing output inside Git repository: " + canonical);
        return canonical;
    }
    public void run() throws Exception {
        File out=guardedOutput(new File(getScriptArgs()[0])); if(!out.mkdirs() && !out.isDirectory()) throw new Exception("output directory");
        var targets=new LinkedHashSet<Function>();
        String[] strings={"native reader resource scope begin", "hook context allocation failed", "slow reader", "native DirectInput device capabilities unavailable", "native added-hook batch failed"};
        for(String value:strings) {
            var start=currentProgram.getMinAddress();
            while(start!=null) {
                var found=currentProgram.getMemory().findBytes(start,value.getBytes(StandardCharsets.US_ASCII),null,true,monitor);
                if(found==null) break;
                var refs=currentProgram.getReferenceManager().getReferencesTo(found);
                while(refs.hasNext()) {
                    var function=getFunctionContaining(refs.next().getFromAddress());
                    if(function!=null) targets.add(function);
                }
                start=found.add(1);
            }
        }
        var decompiler=new DecompInterface(); decompiler.openProgram(currentProgram);
        try(PrintWriter writer=new PrintWriter(new File(out,"shipped_threading.txt"),"UTF-8")) {
            for(var function:targets) {
                writer.println("// "+function.getName()+" "+function.getEntryPoint());
                var result=decompiler.decompileFunction(function,0,monitor);
                writer.println(result.decompileCompleted()?result.getDecompiledFunction().getC():result.getErrorMessage());
            }
        } finally { decompiler.dispose(); }
        println("Shipped threading functions retained: "+targets.size());
    }
}
