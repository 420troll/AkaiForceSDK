import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

/** Mark a caller-supplied reset vector target before Ghidra auto-analysis. */
public class Dp4MarkReset extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) {
            throw new IllegalArgumentException("expected reset-vector address, e.g. 0xfffe");
        }
        Address vector = toAddr(Long.decode(args[0]));
        int high = getByte(vector) & 0xff;
        int low = getByte(vector.add(1)) & 0xff;
        Address entry = toAddr((high << 8) | low);
        currentProgram.getSymbolTable().addExternalEntryPoint(entry);
        boolean decoded = disassemble(entry);
        Function function = createFunction(entry, null);
        println("RESET_VECTOR vector=" + vector +
            " value=" + String.format("%02x%02x", high, low) +
            " entry=" + entry + " disassembled=" + decoded +
            " function=" + (function != null));
    }
}
