"""Run with an installed lz4ext: python tests/bench.py --loops 1000."""
import argparse
from pathlib import Path
from timeit import timeit
import lz4ext


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--loops", type=int, default=1000)
    parser.add_argument("--input", type=Path,
                        default=Path(__file__).resolve().parents[1] / "src" / "lz4.c")
    args = parser.parse_args()
    if args.loops <= 0:
        parser.error("--loops must be positive")
    data = args.input.read_bytes()
    encoded = lz4ext.compress(data)
    decode = lambda: lz4ext.decompress(encoded, max_output_size=max(1, len(data)))
    assert decode() == data
    print(f"Input: {len(data)} bytes; compressed: {len(encoded)} bytes")
    print(f"Compression ({args.loops} calls): "
          f"{timeit(lambda: lz4ext.compress(data), number=args.loops):.6f}s")
    print(f"Decompression ({args.loops} calls): "
          f"{timeit(decode, number=args.loops):.6f}s")


if __name__ == "__main__":
    main()
