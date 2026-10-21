// Offline copied Wakka modules; never load/execute a game DLL.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import java.io.File;
import java.io.PrintWriter;
public class wakka_module extends GhidraScript {
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
        if(args.length!=1) throw new IllegalArgumentException("output directory required");
        File out=guardedOutput(new File(args[0])); if(!out.mkdirs() && !out.isDirectory()) throw new Exception("output directory");
        DecompInterface decompiler=new DecompInterface(); decompiler.openProgram(currentProgram);
        // GetEffectOverlayTable points to callbacks that have no direct callers.
        // These entries are confirmed by its table and native table mutations.
        boolean hd=currentProgram.getMemory().getByte(toAddr(0x10004288L))==(byte)0xb8;
        long[] callbacks=hd?
            new long[]{0x10005390L,0x10005480L,0x10005500L,0x10005550L,0x10005570L}:
            new long[]{0x10001180L,0x10001270L,0x100012f0L,0x10001340L,0x10001360L};
        for(long value:callbacks) if(getFunctionAt(toAddr(value))==null) { disassemble(toAddr(value)); createFunction(toAddr(value),null); }
        try(PrintWriter writer=new PrintWriter(new File(out,currentProgram.getName()+".txt"),"UTF-8")) {
            var functions=currentProgram.getFunctionManager().getFunctions(true);
            while(functions.hasNext() && !monitor.isCancelled()) {
                var function=functions.next();
                if(function.isExternal()) continue;
                writer.println("// Function: "+function.getName()+" // Address: "+function.getEntryPoint());
                DecompileResults result=decompiler.decompileFunction(function,0,monitor);
                writer.println(result.decompileCompleted()?result.getDecompiledFunction().getC():result.getErrorMessage());
            }
        } finally { decompiler.dispose(); }
        println("Wakka module export complete: "+out.getAbsolutePath());
    }
}
