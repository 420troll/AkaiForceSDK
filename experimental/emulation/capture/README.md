# Local ES5510 capture inspection

`tools/es5510_capture_inspect.py` accepts either:

1. ASCII trace lines emitted by the EPS-16 Plus emulator when its local
   `EPS16_TRACE_ES5510_PROGRAM` diagnostic is enabled; or
2. A neutral text list with one 48-bit word per line, optionally as
   `index: 0123456789ab` or `0x0123456789ab`. `#` comments are ignored.

Example:

```sh
python3 tools/es5510_capture_inspect.py /path/to/local-es5510-trace.txt
python3 tools/es5510_capture_inspect.py /path/to/local-es5510-trace.txt \
  --json-output /path/to/local-inspection.json
```

The tool checks syntax, contiguous indices, the 160-word program capacity,
48-bit width, and potential END opcodes. It does not accept ROM, OS disk,
firmware, Waveboy effect files, or undocumented binary containers. It does not
execute the result. The optional JSON contains the instruction words and can
therefore contain copyrighted effect material; keep it local unless you have
rights to distribute that data.

The EPS trace's `gpr` field is the value of `es5510_gpr[index]` printed beside
each program word. It is **not** the complete GPR image and is not a valid
substitute for effect initialization. The trace does not include delay DRAM,
serial routing, host-serial configuration, parameter state, or all host
transactions. An instruction listing alone is not sufficient to replay an
EPS/Waveboy effect as an audio program.

The test suite uses generated synthetic records only:

```sh
python3 -m unittest discover -s experimental/emulation/capture/tests -v
```
