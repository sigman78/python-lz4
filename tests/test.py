import lz4ext
import sys


import unittest
import os
import ctypes

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

if __name__ == '__main__':
    unittest.main()

