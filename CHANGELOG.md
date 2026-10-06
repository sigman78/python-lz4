# Changelog

## 0.8.0

- Update vendored LZ4 r119 to official v1.10.0.
- Require CPython 3.11 or newer and contiguous binary input; reject text.
- Check compression bounds and pass explicit output capacities.
- Validate prefixed decoded lengths and empty framing; emit canonical empty blocks.
- Return cleanly from raw decoder errors and check output capacity arithmetic.
- Default decoder allocation limit to 64 MiB, with keyword-only `max_output_size`
  overrides on every decoder alias.
- Preserve the four-byte little-endian prefixed format and existing aliases.
- Use PEP 517/621 packaging and include binding and vendored LZ4 licenses.
- Expose package/library versions and default output limit.

Historical malformed 20-byte empty records require trusted offline migration;
see the README. Valid nonempty legacy blocks remain readable.
