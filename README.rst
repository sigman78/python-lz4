=============
python-lz4ext
=============

``lz4ext`` provides native LZ4 block compression for CPython 3.11–3.14,
using vendored `LZ4 v1.10.0 <https://github.com/lz4/lz4/releases/tag/v1.10.0>`_.
The distribution and import name are both ``lz4ext``.

GitHub Actions tests/builds the supported Python matrix. See ``RELEASING.md``
for release artifacts and the separate PyPI Trusted Publishing configuration,
and ``SECURITY.md`` for vulnerability reporting.

Install and develop
===================

Install the distribution::

    python -m pip install lz4ext

Build from this checkout (requires a C compiler, such as MSVC on Windows)::

    python -m pip install .
    python tests/test.py
    python tests/bench.py --loops 1000

Build distributable artifacts with an isolated PEP 517 backend::

    python -m pip install build
    python -m build

The extension uses the CPython C API, retains the GIL, and does not declare
free-threaded Python support. Builds use compiler defaults without
``-march=native`` flags. No runtime Python dependencies are required.

API
===

Input must be contiguous bytes-like data, including ``bytes``, ``bytearray``,
and contiguous ``memoryview``. Text is rejected; encode it explicitly.
All output is ``bytes``::

    import lz4ext
    data = b"hello" * 100
    block = lz4ext.compress(data)
    assert lz4ext.decompress(block) == data
    assert lz4ext.loads(lz4ext.compressHC(data)) == data

``compress(data)`` and ``compressHC(data)`` return a four-byte little-endian
unsigned original size followed by an LZ4 block. This is the historical
python-lz4ext format, not the LZ4 frame format.

``decompress(data, *, max_output_size=67108864)`` validates framing and requires
an exact match between the decoded length and header. ``max_output_size`` must
be positive and at most ``INT_MAX`` (2,147,483,647). Declared output above the
limit is rejected before allocation. To deliberately permit larger output,
pass an explicit limit appropriate to the data and available memory.

``compress_raw(data)`` returns a headerless block.
``decompress_raw(data, output_size=0, *, max_output_size=67108864)`` allocates
the supplied capacity, or twice the compressed input length when capacity is
zero or omitted. Capacity must fit the output limit. High-ratio blocks usually
need an explicit capacity::

    raw = lz4ext.compress_raw(data)
    assert lz4ext.decompress_raw(raw, len(data)) == data

Malformed raw input and insufficient capacity both raise ``ValueError``;
the decoder cannot distinguish them. There are no automatic allocation retries.
Empty raw output is ``b'\x00'``; zero-length raw input is invalid.

Compression rejects input above LZ4's supported maximum (2,113,929,216 bytes).
Limits do not guarantee allocation success; ``MemoryError`` may still occur.
Output caps bound the allocation for a call, not total process memory or
cumulative work. This block format provides no checksum or authenticity check.

Aliases and constants
=====================

``dumps`` and ``LZ4_compress`` behave like ``compress``.
``loads``, ``uncompress``, and ``LZ4_uncompress`` behave like ``decompress``.
``uncompress_raw`` behaves like ``decompress_raw``. All decoder aliases accept
the keyword-only output limit.

``VERSION`` and ``__version__`` contain the package version. ``LZ4_VERSION``
reports the linked vendored library version. ``DEFAULT_MAX_OUTPUT_SIZE`` is
64 MiB (67,108,864 bytes).

Compatibility and migration
===========================

Version 0.8.0 requires Python 3.11 or newer and rejects implicit text input.
Valid r119-era nonempty prefixed and raw blocks remain readable; compression
may produce different bytes after the library upgrade. No frame API is added.
Output above 64 MiB needs an explicit limit override. Incorrect size headers
and detected malformed blocks now raise exceptions.

Empty prefixed output is exactly five zero bytes (zero header plus canonical
``00`` block). The exact four-byte zero-header legacy sentinel is accepted.
Old releases could emit 20-byte empty records: a zero header followed by 16
uninitialized bytes. These malformed records are rejected. If trusted records
are known independently to represent empty data, migrate them offline to the
canonical five-byte encoding. Arbitrary zero-header padding is not valid data.

Licenses
========

The binding uses BSD 3-Clause in ``LICENSE``. The unmodified vendored LZ4
sources use BSD 2-Clause; see ``src/LICENSE.lz4`` and ``src/README.lz4.md`` for
upstream provenance. Both licenses accompany source and wheel distributions.
