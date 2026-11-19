import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.block.BasicBlockModel;
import ghidra.program.model.block.CodeBlock;
import ghidra.program.model.block.CodeBlockReference;
import ghidra.program.model.block.CodeBlockReferenceIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import java.io.FileWriter;
import java.io.PrintWriter;

/** Write bounded control-flow and memory-reference evidence to a caller-owned path. */
public class Dp4Report extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("expected private report path");
        try (PrintWriter out = new PrintWriter(new FileWriter(args[0]))) {
            out.println("PROGRAM " + currentProgram.getName());
            out.println("LANGUAGE " + currentProgram.getLanguage().getLanguageID());
            out.println("FUNCTION_COUNT " + currentProgram.getFunctionManager().getFunctionCount());
            out.println("CPU_MAP_NOTE Ghidra import blocks below describe the binary loader mapping, not board RAM/ROM types");
            for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
                out.println("GHIDRA_IMPORT_BLOCK " + block.getName() + " " + block.getStart() + ".." +
                    block.getEnd() + " r=" + block.isRead() + " w=" + block.isWrite() +
                    " x=" + block.isExecute());
            }

            long[] sites = {0x84e4, 0x85e7, 0x85ea, 0x85e0, 0xb342, 0xb359,
                0xb36b, 0xb35c, 0xb373, 0xb385};
            for (long site : sites) {
                Address address = toAddr(site);
                Instruction instruction = currentProgram.getListing().getInstructionAt(address);
                Function function = currentProgram.getFunctionManager().getFunctionContaining(address);
                out.println("SITE " + address + " instruction=" +
                    (instruction == null ? "<none>" : instruction.toString()) +
                    " flow=" + (instruction == null ? "<none>" : instruction.getFlowType()) +
                    " function=" + (function == null ? "<none>" : function.getEntryPoint()));
                if (instruction != null) {
                    for (Reference ref : instruction.getReferencesFrom()) {
                        out.println("  REF " + ref.getFromAddress() + " -> " + ref.getToAddress() +
                            " " + ref.getReferenceType());
                    }
                }
                if (function != null) {
                    Address start = address.subtract(8);
                    Address end = address.add(24);
                    InstructionIterator around = currentProgram.getListing().getInstructions(
                        new AddressSet(start, end), true);
                    while (around.hasNext()) {
                        Instruction line = around.next();
                        out.println("  INS " + line.getAddress() + " " + line);
                    }
                }
            }

            // Preserve enough caller context to audit X-register provenance around
            // the known computed calls in each firmware revision.
            for (long site : new long[] {0x85e7, 0x85ea}) {
                Instruction call = currentProgram.getListing().getInstructionAt(toAddr(site));
                if (call == null || !call.getFlowType().isComputed()) continue;
                Function owner = currentProgram.getFunctionManager()
                    .getFunctionContaining(toAddr(site));
                out.println("INDIRECT_CALL " + toAddr(site) + " " + call +
                    " owner=" + (owner == null ? "<none>" : owner.getEntryPoint()));
                Address start = toAddr(site).subtract(0x70);
                Address end = toAddr(site).add(8);
                InstructionIterator context = currentProgram.getListing().getInstructions(
                    new AddressSet(start, end), true);
                while (context.hasNext()) {
                    Instruction line = context.next();
                    out.println("X_CONTEXT " + line.getAddress() + " " + line);
                }
            }

            for (long peripheral : new long[] {0x1800, 0x1801}) {
                Address address = toAddr(peripheral);
                out.println("XREFS_TO " + address);
                ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(address);
                while (refs.hasNext()) {
                    Reference ref = refs.next();
                    out.println("  XREF " + ref.getFromAddress() + " " + ref.getReferenceType());
                }
            }

            InstructionIterator all = currentProgram.getListing().getInstructions(true);
            int instructionCount = 0;
            // These are values listed in the service-manual ESP event table.
            // A matching immediate is only a search candidate, not proof that
            // this instruction generates a diagnostic event.
            int[] manualEventCodeValues = {0x30, 0x31, 0x32, 0x33, 0x35, 0x36, 0x37, 0x38,
                0x40, 0x41, 0x42, 0x43, 0x45, 0x46, 0x47, 0x48,
                0x50, 0x51, 0x52, 0x53, 0x60, 0x61, 0x62, 0x63,
                0x70, 0x71, 0x72, 0x73, 0x80, 0x81, 0x82, 0x83,
                0x85, 0x86, 0x87, 0x88, 0x90, 0x91, 0x92, 0x93};
            while (all.hasNext()) {
                Instruction instruction = all.next();
                instructionCount++;
                out.println("INSTRUCTION_BOUNDARY " + instruction.getAddress());
                String mnemonic = instruction.getMnemonicString();
                String operands = instruction.getDefaultOperandRepresentation(0);
                if ((operands.contains(",X") || operands.contains(", X")) &&
                    (operands.startsWith("0x7") || operands.startsWith("7,"))) {
                    out.println("INDEXED_X_OFFSET7 " + instruction.getAddress() + " " + instruction);
                }
                for (int operand = 0; operand < instruction.getNumOperands(); operand++) {
                    String representation = instruction.getDefaultOperandRepresentation(operand);
                    for (int code : manualEventCodeValues) {
                        if (representation.equalsIgnoreCase("#0x" + Integer.toHexString(code))) {
                            out.println("IMMEDIATE_CONSTANT_CANDIDATE " + instruction.getAddress() +
                                " code=0x" + Integer.toHexString(code) + " " + instruction);
                            break;
                        }
                    }
                }
                if (instruction.getFlowType().isComputed() && !mnemonic.equals("SWI")) {
                    out.println("COMPUTED_FLOW " + instruction.getAddress() + " " + instruction);
                }
                if (mnemonic.equals("ABX") || mnemonic.equals("MUL") ||
                    mnemonic.equals("XGDX") || mnemonic.equals("SLP")) {
                    Function owner = currentProgram.getFunctionManager()
                        .getFunctionContaining(instruction.getAddress());
                    out.println("6801_FAMILY_INSN " + instruction.getAddress() + " " + instruction +
                        " function=" + (owner == null ? "<none>" : owner.getEntryPoint()));
                }
            }
            out.println("INSTRUCTION_COUNT " + instructionCount);

            for (long target : new long[] {0xb342, 0xb35c, 0xb359, 0xb373,
                                           0xb36b, 0xb385, 0xce97, 0xce9f}) {
                Address address = toAddr(target);
                out.println("XREFS_TO " + address);
                ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(address);
                while (refs.hasNext()) {
                    Reference ref = refs.next();
                    out.println("  XREF " + ref.getFromAddress() + " " + ref.getReferenceType());
                }
            }

            BasicBlockModel model = new BasicBlockModel(currentProgram);
            for (long site : new long[] {0x85e7, 0x85ea}) {
                CodeBlock[] blocks = model.getCodeBlocksContaining(toAddr(site), monitor);
                for (CodeBlock block : blocks) {
                    out.println("BLOCK_FOR " + toAddr(site) + " " + block.getMinAddress() + ".." +
                        block.getMaxAddress());
                    CodeBlockReferenceIterator destinations = block.getDestinations(monitor);
                    while (destinations.hasNext()) {
                        CodeBlockReference edge = destinations.next();
                        out.println("  EDGE " + edge.getFlowType() + " -> " +
                            edge.getDestinationAddress());
                    }
                }
            }
        }
        println("DP4_REPORT_WRITTEN " + args[0]);
    }
}
