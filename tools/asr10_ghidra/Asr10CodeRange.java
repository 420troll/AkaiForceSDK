import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;

/** Print a bounded, already-mapped 68000 ROM range with file-offset correlation. */
public class Asr10CodeRange extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        long start = args.length > 0 ? Long.decode(args[0]) : 0x17340;
        long length = args.length > 1 ? Long.decode(args[1]) : 0x200;
        Address first = toAddr(start);
        Address last = first.add(length - 1);
        clearListing(first, last);
        long cursor = start;
        int count = 0;
        while (cursor < start + length && !monitor.isCancelled()) {
            Address address = toAddr(cursor);
            Instruction instruction = getInstructionAt(address);
            if (instruction == null) {
                disassemble(address);
                instruction = getInstructionAt(address);
            }
            if (instruction == null) {
                cursor += 2;
                continue;
            }
            println(String.format("ROM_OFF_%06X CPU_ALIAS_%08X %s", cursor,
                0xF80000L + cursor, instruction));
            cursor = instruction.getMaxAddress().getOffset() + 1;
            count++;
        }
        println(String.format("DECODED_INSTRUCTIONS=%d FILE_OFFSET=0x%X LENGTH=0x%X",
            count, start, length));
    }
}
