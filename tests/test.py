import lz4ext
import sys


import unittest
import os
import ctypes
import subprocess

class TestLZ4ext(unittest.TestCase):

    def test_random(self):
      DATA = os.urandom(128 * 1024)  # Read 128kb
      self.assertEqual(DATA, lz4ext.loads(lz4ext.dumps(DATA)))

    def test_raw(self):
      DATA = b"abc def"
      self.assertEqual(DATA, lz4ext.decompress_raw(lz4ext.compress_raw(DATA), 1024))

    def test_zero_offset_does_not_expose_destination_memory(self):
      # A zero match offset must not copy uninitialized destination bytes.
      block = bytes.fromhex('0f000005804142434445464748')
      expected = b'\x00' * 24 + b'ABCDEFGH'
      self.assertEqual(expected, lz4ext.decompress_raw(block, 32))

    def test_resized_bytes_have_c_api_terminator(self):
      as_string = ctypes.pythonapi.PyBytes_AsString
      as_string.argtypes = [ctypes.py_object]
      as_string.restype = ctypes.c_void_p
      for size in range(1, 33):
        data = bytes(range(1, size + 1))
        for compress in (lz4ext.compress, lz4ext.compressHC,
                         lz4ext.compress_raw):
          encoded = compress(data)
          self.assertEqual(0, ctypes.c_ubyte.from_address(
              as_string(encoded) + len(encoded)).value)
        encoded = lz4ext.compress_raw(data)
        decoded = lz4ext.decompress_raw(encoded, size + 1)
        self.assertEqual(data, decoded)
        self.assertEqual(0, ctypes.c_ubyte.from_address(
            as_string(decoded) + len(decoded)).value)

    def test_prefixed_size_must_match_decoded_size(self):
      block = lz4ext.compress(b'abc def')
      for declared_size in (1, 6, 8, 32):
        with self.subTest(declared_size=declared_size):
          with self.assertRaises(ValueError):
            lz4ext.decompress(declared_size.to_bytes(4, 'little') + block[4:])

    def test_prefixed_empty_encodings(self):
      for block in (b'\x00' * 4, b'\x00' * 5):
        self.assertEqual(b'', lz4ext.decompress(block))
      for tail in (b'garbage', b'\x01', b'\x00\x00', b'\x00garbage'):
        with self.subTest(tail=tail):
          with self.assertRaises(ValueError):
            lz4ext.decompress(b'\x00' * 4 + tail)

    def test_empty_compression_is_canonical(self):
      for compress in (lz4ext.compress, lz4ext.compressHC):
        with self.subTest(compress=compress.__name__):
          encoded = compress(b'')
          self.assertEqual(b'\x00' * 5, encoded)
          self.assertEqual(b'', lz4ext.decompress(encoded))
      encoded = lz4ext.compress_raw(b'')
      self.assertEqual(b'\x00', encoded)
      self.assertEqual(b'', lz4ext.decompress_raw(encoded))

    def test_raw_errors_do_not_crash(self):
      script = '''
import lz4ext
data = b'A' * 4096
encoded = lz4ext.compress_raw(data)
for block, capacity in ((b'', 0), (b'\\xff', 32),
                        (encoded, -1), (encoded, 0), (encoded, 32)):
    try:
        lz4ext.decompress_raw(block, capacity)
    except ValueError:
        pass
    else:
        raise AssertionError('invalid block/capacity was accepted')
assert lz4ext.decompress_raw(encoded, len(data)) == data
'''
      completed = subprocess.run([sys.executable, '-c', script],
                                 capture_output=True, text=True)
      self.assertEqual(0, completed.returncode, completed.stderr)

    def test_compression_boundary_roundtrips(self):
      for size in (0, 1, 16, 64 * 1024):
        data = (b'abcdefgh' * ((size + 7) // 8))[:size]
        for compress in (lz4ext.compress, lz4ext.compressHC):
          with self.subTest(size=size, compress=compress.__name__):
            self.assertEqual(data, lz4ext.decompress(compress(data)))
        with self.subTest(size=size, compress='compress_raw'):
          self.assertEqual(data, lz4ext.decompress_raw(
              lz4ext.compress_raw(data), max(1, size)))

    def test_decoder_output_limits(self):
      data = b'abc def'
      block = lz4ext.compress(data)
      for decode in (lz4ext.decompress, lz4ext.uncompress, lz4ext.loads,
                     lz4ext.LZ4_uncompress):
        with self.subTest(decode=decode.__name__):
          self.assertEqual(data, decode(block, max_output_size=7))
          with self.assertRaises(ValueError):
            decode(block, max_output_size=6)
          for limit in (0, -1, 2 ** 31):
            with self.assertRaises(ValueError):
              decode(b'\x00' * 5, max_output_size=limit)
          with self.assertRaises(TypeError):
            decode(block, 7)
          for declared_size in (64 * 1024 * 1024 + 1, 2 ** 31 - 1,
                                2 ** 31, 2 ** 32 - 1):
            with self.assertRaises(ValueError):
              decode(declared_size.to_bytes(4, 'little') + b'\x00')
      raw = lz4ext.compress_raw(data)
      for decode in (lz4ext.decompress_raw, lz4ext.uncompress_raw):
        with self.subTest(decode=decode.__name__):
          self.assertEqual(data, decode(raw, 7, max_output_size=7))
          for capacity in (8, 0, 64 * 1024 * 1024 + 1, 2 ** 31 - 1):
            with self.assertRaises(ValueError):
              decode(raw, capacity, max_output_size=7)
          for limit in (0, -1, 2 ** 31):
            with self.assertRaises(ValueError):
              decode(b'\x00', 1, max_output_size=limit)
          with self.assertRaises(TypeError):
            decode(raw, 7, 7)

if __name__ == '__main__':
    unittest.main()

