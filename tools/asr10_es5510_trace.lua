-- Passive ASR-10 ES5510 host-bus recorder for OpenASR/MAME Lua.
-- Install with: -autoboot_script tools/asr10_es5510_trace.lua
-- It only observes CPU program-space accesses; it never writes emulator state.
-- Keep this module-level array alive: MAME Lua taps are GC-owned handles.
local machine = manager.machine
local cpu = machine.devices[":maincpu"]
local program = cpu.spaces["program"]
local taps = {}
local seq = 0
local openasr = "241f9331477b07b072330ca6ebd2d7a2e2cc832e"
local mame = "8be57cdc5f44ab830dff44c06dd39f24190742d4"

local function pc_value()
  local s = cpu.state["PC"] or cpu.state["CURPC"] or cpu.state["GENPC"]
  return s and (s.value & 0x00ffffff) or -1
end

local function emit(kind, address, data, mask)
  seq = seq + 1
  local t = tostring(emu.time())
  print(string.format("T551CAP\t%s\t%d\t%08X\t%08X\t%08X\t%08X\t%s\t%s\t%s",
    "v1", seq, address, data & 0xffffffff, mask & 0xffffffff,
    pc_value() & 0xffffffff, kind, openasr, mame) .. "\t" .. t)
end

local function install(start, stop, kind)
  taps[#taps + 1] = program:install_write_tap(start, stop, "t551_" .. kind,
    function(offset, data, mask)
      emit(kind, offset, data, mask)
    end)
end

local function install_read(start, stop, kind)
  taps[#taps + 1] = program:install_read_tap(start, stop, "t551_" .. kind,
    function(offset, data, mask)
      emit(kind, offset, data, mask)
      return nil
    end)
end

-- Latch bytes and all four fixed host selector/commit addresses.
install(0x00fc3000, 0x00fc303f, "host_w")
install_read(0x00fc3000, 0x00fc303f, "host_r")
install(0x00fc3100, 0x00fc3101, "read_select_w")
install_read(0x00fc3100, 0x00fc3101, "read_select_r")
install(0x00fc3140, 0x00fc3141, "gpr_commit_w")
install_read(0x00fc3140, 0x00fc3141, "gpr_commit_r")
install(0x00fc3180, 0x00fc3181, "instr_commit_w")
install_read(0x00fc3180, 0x00fc3181, "instr_commit_r")
install(0x00fc31c0, 0x00fc31c1, "combined_commit_w")
install_read(0x00fc31c0, 0x00fc31c1, "combined_commit_r")
-- PADAT contains PA4. This observation is a firmware register write; the
-- relationship to RUN/HALT is the OpenASR board model, not a physical pin trace.
install(0x00fc6822, 0x00fc6823, "padat_w")

print("T551CAP_META\t1\tsource_class=mame_runtime_firmware_write_observation\tOpenASR=" .. openasr .. "\tMAME=" .. mame ..
  "\tmachine=asr10booth\tobservation=passive_cpu_program_space_taps\trom_identity=not_verified")
