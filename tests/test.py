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

if __name__ == '__main__':
    unittest.main()

