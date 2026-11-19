// Private, read-only Ghidra audit for DP/4 bank-latch and U54-window references.
// Reports contain proprietary firmware addresses/disassembly; keep output outside Git.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import java.io.FileWriter;
import java.io.PrintWriter;

public class Dp4BankPathAudit extends GhidraScript {
    private PrintWriter out;

    private void context(Address site, int before, int after) {
        Address start = site.subtract(before);
        Address end = site.add(after);
        InstructionIterator it = currentProgram.getListing().getInstructions(
            new AddressSet(start, end), true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            out.println("  INS " + ins.getAddress() + " " + ins);
        }
    }

    private void refsTo(Address target, String label, boolean showContext) {
        out.println("TARGET " + label + " " + target);
        ReferenceIterator it = currentProgram.getReferenceManager().getReferencesTo(target);
        while (it.hasNext()) {
            Reference ref = it.next();
            Address from = ref.getFromAddress();
            Instruction ins = currentProgram.getListing().getInstructionAt(from);
            Function fn = currentProgram.getFunctionManager().getFunctionContaining(from);
            out.println("  REF " + from + " " + ref.getReferenceType() +
                " ins=" + (ins == null ? "<none>" : ins) +
                " function=" + (fn == null ? "<none>" : fn.getEntryPoint()));
            if (showContext && ins != null) context(from, 12, 8);
        }
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 3) {
            throw new IllegalArgumentException("expected output-path bank-helper-entry bank-write-address");
        }
        Address helper = toAddr(Long.parseLong(args[1], 16));
        Address bankWrite = toAddr(Long.parseLong(args[2], 16));
        try (PrintWriter writer = new PrintWriter(new FileWriter(args[0]))) {
            out = writer;
            out.println("PROGRAM " + currentProgram.getName());
            refsTo(helper, "BANK_HELPER_ENTRY", true);
            refsTo(bankWrite, "BANK_LATCH_WRITE", true);
            long[] bankRelated = {0xbf2d, 0xbf40, 0xbf47, 0xbf5a,
                0x0092, 0x0093, 0x0094, 0x00b9, 0x0002};
            String[] labels = {"BANK_STATE_ROUTINE_A", "BANK_STATE_ROUTINE_B",
                "BANK_STATE_ROUTINE_C", "BANK_STATE_ROUTINE_D", "LATCH_SHADOW",
                "BANK_VALUE_A", "BANK_VALUE_B", "BANK_VALUE_C", "PORT_REGISTER_2"};
            for (int i = 0; i < bankRelated.length; i++)
                refsTo(toAddr(bankRelated[i]), labels[i], true);
            refsTo(toAddr(0x0600), "ALL_0600_WRITES", true);
            out.println("DIRECT_REFERENCES_IN_U54_CANDIDATE_WINDOW");
            for (long value = 0x2000; value <= 0x3fff; value++) {
                Address target = toAddr(value);
                ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(target);
                while (refs.hasNext()) {
                    Reference ref = refs.next();
                    Instruction ins = currentProgram.getListing().getInstructionAt(ref.getFromAddress());
                    Function fn = currentProgram.getFunctionManager()
                        .getFunctionContaining(ref.getFromAddress());
                    out.println("U54_CANDIDATE_REF " + ref.getFromAddress() + " -> " + target +
                        " " + ref.getReferenceType() + " ins=" +
                        (ins == null ? "<none>" : ins) + " function=" +
                        (fn == null ? "<none>" : fn.getEntryPoint()));
                }
            }
        }
        println("DP4_BANK_PATH_AUDIT_WRITTEN " + args[0]);
    }
}
