// Private Ghidra report for a specific indirect transfer and its recovered target.
// Output contains copyrighted firmware disassembly/decompilation; keep it private.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import java.io.FileWriter;
import java.io.PrintWriter;

public class Dp4IndirectTargetAudit extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) throw new IllegalArgumentException("expected output-path source-address-hex");
        Address site = toAddr(Long.parseLong(args[1], 16));
        try (PrintWriter out = new PrintWriter(new FileWriter(args[0]))) {
            Instruction ins = currentProgram.getListing().getInstructionAt(site);
            Function fn = currentProgram.getFunctionManager().getFunctionContaining(site);
            out.println("PROGRAM " + currentProgram.getName());
            out.println("SITE " + site + " ins=" + (ins == null ? "<none>" : ins) +
                " function=" + (fn == null ? "<none>" : fn.getEntryPoint()));
            if (ins != null) for (Reference ref : ins.getReferencesFrom())
                out.println("REF " + ref.getFromAddress() + " -> " + ref.getToAddress() + " " + ref.getReferenceType());
            if (fn != null) {
                out.println("CALLERS " + fn.getEntryPoint());
                ReferenceIterator callers = currentProgram.getReferenceManager()
                    .getReferencesTo(fn.getEntryPoint());
                while (callers.hasNext()) {
                    Reference caller = callers.next();
                    Address callSite = caller.getFromAddress();
                    Instruction callIns = currentProgram.getListing().getInstructionAt(callSite);
                    Function callerFn = currentProgram.getFunctionManager()
                        .getFunctionContaining(callSite);
                    out.println("CALLER " + callSite + " " + caller.getReferenceType() +
                        " ins=" + (callIns == null ? "<none>" : callIns) +
                        " function=" + (callerFn == null ? "<none>" : callerFn.getEntryPoint()));
                    if (callIns != null) {
                        InstructionIterator context = currentProgram.getListing().getInstructions(
                            new AddressSet(callSite.subtract(16), callSite.add(8)), true);
                        while (context.hasNext()) {
                            Instruction line = context.next();
                            out.println("  CALLER_INS " + line.getAddress() + " " + line);
                        }
                    }
                }
                out.println("FUNCTION " + fn.getEntryPoint() + ".." + fn.getBody().getMaxAddress());
                InstructionIterator it = currentProgram.getListing().getInstructions(fn.getBody(), true);
                while (it.hasNext()) {
                    Instruction line = it.next();
                    out.println("INS " + line.getAddress() + " " + line);
                }
                DecompInterface decomp = new DecompInterface();
                decomp.openProgram(currentProgram);
                DecompileResults result = decomp.decompileFunction(fn, 30, monitor);
                out.println("DECOMPILED");
                out.println(result.getDecompiledFunction() == null ? result.getErrorMessage() :
                    result.getDecompiledFunction().getC());
                decomp.dispose();
            }
        }
        println("DP4_INDIRECT_TARGET_AUDIT_WRITTEN " + args[0]);
    }
}
