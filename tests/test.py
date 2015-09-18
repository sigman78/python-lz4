import lz4
import sys


import unittest
import os

class TestLZ4(unittest.TestCase):

    def test_random(self):
      DATA = os.urandom(128 * 1024)  # Read 128kb
      self.assertEqual(DATA, lz4.loads(lz4.dumps(DATA)))

    def test_raw(self):
      DATA = b"abc def"
      self.assertEqual(DATA, lz4.decompress_raw(lz4.compress_raw(DATA), 1024))

if __name__ == '__main__':
    unittest.main()

