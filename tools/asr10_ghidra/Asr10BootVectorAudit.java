import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.address.AddressSet;

/** Read the big-endian 68000 reset vectors and seed disassembly at the entry. */
public class Asr10BootVectorAudit extends GhidraScript {
    private long readU32(Address address) throws Exception {
        long value = 0;
        for (int i = 0; i < 4; ++i) value = (value << 8) | (getByte(address.add(i)) & 0xffL);
        return value;
    }

    @Override
    public void run() throws Exception {
        Address vectors = toAddr(0);
        long ssp = readU32(vectors);
        long pc = readU32(vectors.add(4));
        Address entry = toAddr(pc);
        clearListing(entry, entry.add(63));
        currentProgram.getSymbolTable().addExternalEntryPoint(entry);
        boolean decoded = disassemble(entry);
        Function function = currentProgram.getFunctionManager().getFunctionAt(entry);
        if (function == null) function = createFunction(entry, "asr10_reset_entry");
        println(String.format("ASR10_RESET_VECTORS ssp=0x%08x pc=0x%08x decoded=%s function=%s",
            ssp, pc, decoded, function != null));
        StringBuilder bytes = new StringBuilder();
        for (int i = 0; i < 64; ++i) bytes.append(String.format("%02x", getByte(entry.add(i)) & 0xff));
        println("RESET_BYTES " + bytes);
        InstructionIterator instructions = currentProgram.getListing().getInstructions(
            new AddressSet(entry, entry.add(63)), true);
        int count = 0;
        while (count++ < 24 && instructions.hasNext()) {
            Instruction instruction = instructions.next();
            println("RESET_INS " + instruction.getAddress() + " " + instruction);
        }
    }
}
