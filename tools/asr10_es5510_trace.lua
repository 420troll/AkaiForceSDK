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
-- This is the upstream MAME reference commit used by the TROLLfx adapter.
-- OpenASR's ES5510 execution core at its pinned commit is locally modified;
-- the exact file hashes are stated in the metadata below.
local mame = "8be57cdc5f44ab830dff44c06dd39f24190742d4"
local es5510_cpp = "c00a8aff93211383d14c1a3b38d5bcad4c3daabc87d53ee07a595c0c8589ad7a"
local es5510_h = "a29f72169c3e1cbf31d13330d8219acd7d9cd1db6eefe450eb0c16cbbea61cea"
local es5510d_cpp = "4cdf4876bd2a1a8835f405e16eb628ecbd3d84fc364a7b076b44989ad291409e"
local es5510d_h = "8af2daa4ac27eed356a5d278f95605ce0b40e98a0509d538c9a18af53f7c38dc"

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
-- Passive boot-progress context helps distinguish "no effect selected" from
-- a stalled OS boot. These taps do not change controller/CPU state.
install_read(0x000000, 0x00007f, "boot_r")
install(0x00fc4000, 0x00fc4003, "fdc_w")
install_read(0x00fc4000, 0x00fc4003, "fdc_r")
-- PA4 is exposed by OpenASR through an MC68302 output callback, not this
-- program-space map. Capture it with the separate build-only callback log;
-- do not treat a write to fallback RAM at $FC6822 as a PA4 transition.

local last_progress = -1
emu.register_periodic(function()
  local now = emu.time()
  if now >= last_progress + 1 then
    last_progress = math.floor(now)
    print(string.format("T551CAP_PROGRESS\tsec=%.0f\tpc=%08X\tcycles=%s",
      now, pc_value() & 0xffffffff, tostring(cpu.state["CURCYC"] or cpu.state["CYCLES"] or "unavailable")))
  end
end)

print("T551CAP_META\t1\tsource_class=openasr_mame_cpu_runtime_firmware_write_observation\tOpenASR=" .. openasr .. "\tMAME=" .. mame ..
  "\tmachine=asr10booth\tobservation=passive_cpu_program_space_taps\thost_address_encoding=word_base_16bit_active_low_byte\tpa4=separate_model_callback_log\trom_identity=see_separate_run_manifest\t" ..
  "es5510_core_source=openasr_modified\tes5510_cpp_sha256=" .. es5510_cpp .. "\tes5510_h_sha256=" .. es5510_h ..
  "\tes5510d_cpp_sha256=" .. es5510d_cpp .. "\tes5510d_h_sha256=" .. es5510d_h)
