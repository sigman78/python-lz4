# Vendored LZ4

`lz4.c`, `lz4.h`, `lz4hc.c`, and `lz4hc.h` are unmodified copies of
the `lib/` files from the official LZ4 v1.11.1 release:
https://github.com/lz4/lz4/releases/tag/v1.11.1

Upstream commit: `aa4647fad575d01a874aaaadcdbe8e6a40e072f5`.

Retrieved from https://raw.githubusercontent.com/lz4/lz4/v1.11.1/lib/
on 2026-11-26 and verified byte-for-byte against the release tarball
`lz4-1.11.1.tar.gz` (SHA-256
`683fed08a844ad012196381e40badf80c0bbe4962959761416d41c9e67d77ec7`).
The upstream BSD-2-Clause license is in `LICENSE.lz4`.

When updating these files, copy them verbatim from an official release,
update this provenance and the library version in the README/changelog, and
retain the license. The extension reports `LZ4_VERSION` from the linked library.
