// Ghidra headless script for private DP/4 U56 memory-reference analysis.
// Output may contain proprietary firmware addresses and must stay outside Git.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.symbol.Reference;
import java.io.FileWriter;
import java.io.PrintWriter;

/** Emit direct data-reference source/target pairs to a caller-owned private path. */
public class Dp4MemoryAudit extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) {
            throw new IllegalArgumentException("expected private report path");
        }
        try (PrintWriter out = new PrintWriter(new FileWriter(args[0]))) {
            out.println("PROGRAM " + currentProgram.getName());
            out.println("LANGUAGE " + currentProgram.getLanguage().getLanguageID());
            InstructionIterator instructions = currentProgram.getListing().getInstructions(true);
            int count = 0;
            while (instructions.hasNext()) {
                Instruction instruction = instructions.next();
                for (Reference reference : currentProgram.getReferenceManager()
                        .getReferencesFrom(instruction.getAddress())) {
                    if (!reference.getReferenceType().isData()) continue;
                    out.println("DATA_REFERENCE " + instruction.getAddress() + " -> " +
                        reference.getToAddress() + " " + reference.getReferenceType());
                    count++;
                }
            }
            out.println("DATA_REFERENCE_COUNT " + count);
        }
        println("DP4_MEMORY_AUDIT_WRITTEN " + args[0]);
    }
}
