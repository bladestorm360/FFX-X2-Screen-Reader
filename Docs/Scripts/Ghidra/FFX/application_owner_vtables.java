// Read-only FFX application-owner evidence from the existing analyzed project.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.program.model.symbol.Symbol;
import java.io.File;
import java.io.PrintWriter;
import java.text.SimpleDateFormat;
import java.util.Date;
public class application_owner_vtables extends GhidraScript {
    private File guardedOutput(File output) throws Exception {
        String root = System.getenv("FFX_EVIDENCE_ROOT");
        if (root == null || root.trim().isEmpty()) throw new IllegalStateException("Set FFX_EVIDENCE_ROOT or run the guarded Ghidra wrapper.");
        File canonical = output.getCanonicalFile();
        for (File current = canonical; current != null; current = current.getParentFile())
            if (new File(current, ".git").exists() || (new File(current, "HEAD").isFile() && new File(current, "objects").isDirectory())) throw new IllegalArgumentException("Refusing output inside Git repository: " + canonical);
        return canonical;
    }
    public void run() throws Exception {
        currentProgram.setTemporary(true);
        File directory=guardedOutput(new File(getScriptArgs()[0])); if(!directory.mkdirs() && !directory.isDirectory()) throw new Exception("output directory");
        String stamp=new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date());
        File table=new File(directory,"application_vtables_"+stamp+".tsv");
        File bodies=new File(directory,"application_callbacks_"+stamp+".txt");
        if(!table.createNewFile() || !bodies.createNewFile()) throw new Exception("Refusing to overwrite prior evidence");
        int found=0;
        long base=currentProgram.getImageBase().getOffset();
        DecompInterface decompiler=new DecompInterface();decompiler.openProgram(currentProgram);
        try(PrintWriter rows=new PrintWriter(table,"UTF-8");PrintWriter text=new PrintWriter(bodies,"UTF-8")) {
            rows.println("program\tsymbol\tvtable_rva\tslot\tfunction_rva\tfunction");
            var symbols=currentProgram.getSymbolTable().getAllSymbols(true);
            while(symbols.hasNext() && !monitor.isCancelled()) {
                Symbol symbol=symbols.next();String name=symbol.getName(true);
                if(!name.contains("FFXApplication") || !name.contains("vftable")) continue;
                ++found;
                for(int slot=0;slot<24 && !monitor.isCancelled();++slot) {
                    long pointer=Integer.toUnsignedLong(currentProgram.getMemory().getInt(symbol.getAddress().add(slot*4)));
                    var function=getFunctionAt(toAddr(pointer));
                    rows.println(currentProgram.getName()+"\t"+name+"\t"+Long.toHexString(symbol.getAddress().getOffset()-base)+"\t"+slot+"\t"+Long.toHexString(pointer-base)+"\t"+(function==null?"unresolved":function.getName(true)));
                    if(function==null || (slot!=4 && slot!=7 && slot!=9)) continue;
                    text.println("// Symbol="+name+" slot="+slot+" function="+function.getName(true)+" RVA="+Long.toHexString(pointer-base));
                    var result=decompiler.decompileFunction(function,0,monitor);
                    text.println(result.decompileCompleted()?result.getDecompiledFunction().getC():result.getErrorMessage());
                }
            }
        } finally {decompiler.dispose();}
        println("Read-only FFX application vtables="+found+" table="+table.getAbsolutePath()+" callbacks="+bodies.getAbsolutePath());
    }
}
