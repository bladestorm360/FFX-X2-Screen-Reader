// Targeted offline analysis; read-only project, timestamped output directory.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.File;
import java.io.PrintWriter;
public class overdrive_targets extends GhidraScript {
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
        long[] targets={0x890de0L,0x890e80L,0x898d30L,0x8981a0L,0x897730L,0x8976f0L,
            0x898e80L,0x898ea0L,0x8e8980L,0x8e89d0L,0x906420L,0x906680L,0x9066b0L,0x9066d0L};
        try(PrintWriter writer=new PrintWriter(new File(out,"overdrive.txt"),"UTF-8")) {
            for(long value:targets) {
                Address address=toAddr(value);
                Function function=getFunctionAt(address);
                if(function==null) { disassemble(address); function=createFunction(address,null); }
                writer.println("// Address: "+address);
                if(function==null) { writer.println("UNRESOLVED"); continue; }
                DecompileResults result=decompiler.decompileFunction(function,0,monitor);
                writer.println(result.decompileCompleted()?result.getDecompiledFunction().getC():result.getErrorMessage());
                for(var reference:getReferencesTo(address)) writer.println("// Xref: "+reference.getFromAddress());
            }
        } finally { decompiler.dispose(); }
        println("Overdrive export complete: "+out.getAbsolutePath());
    }
}
