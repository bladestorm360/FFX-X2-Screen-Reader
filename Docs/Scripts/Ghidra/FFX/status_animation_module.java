// Offline, user-authorized copies. Never loads or executes an animation DLL.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import java.io.File;
import java.io.PrintWriter;
public class status_animation_module extends GhidraScript {
    private File guardedOutput(File output) throws Exception {
        String root = System.getenv("FFX_EVIDENCE_ROOT");
        if (root == null || root.trim().isEmpty()) throw new IllegalStateException("Set FFX_EVIDENCE_ROOT or run the guarded Ghidra wrapper.");
        File canonical = output.getCanonicalFile();
        for (File current = canonical; current != null; current = current.getParentFile())
            if (new File(current, ".git").exists() || (new File(current, "HEAD").isFile() && new File(current, "objects").isDirectory())) throw new IllegalArgumentException("Refusing output inside Git repository: " + canonical);
        return canonical;
    }
    public void run() throws Exception {
        String[] args=getScriptArgs();
        if(args.length<2) throw new IllegalArgumentException("output and native callback entries required");
        File out=guardedOutput(new File(args[0])); if(!out.mkdirs() && !out.isDirectory()) throw new Exception("output directory");
        for(int i=1;i<args.length;++i) {
            var entry=toAddr(Long.parseUnsignedLong(args[i],16));
            if(getFunctionContaining(entry)==null) { disassemble(entry); createFunction(entry,null); }
        }
        DecompInterface decompiler=new DecompInterface(); decompiler.openProgram(currentProgram);
        try(PrintWriter writer=new PrintWriter(new File(out,currentProgram.getName()+".txt"),"UTF-8")) {
            var functions=currentProgram.getFunctionManager().getFunctions(true);
            while(functions.hasNext() && !monitor.isCancelled()) {
                var function=functions.next(); if(function.isExternal()) continue;
                writer.println("// Function: "+function.getName()+" // Address: "+function.getEntryPoint());
                DecompileResults result=decompiler.decompileFunction(function,0,monitor);
                writer.println(result.decompileCompleted()?result.getDecompiledFunction().getC():result.getErrorMessage());
            }
        } finally { decompiler.dispose(); }
        println("Status animation export complete: "+currentProgram.getName());
    }
}
