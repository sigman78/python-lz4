import lz4ext
import sys


import unittest
import os

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

if __name__ == '__main__':
    unittest.main()

