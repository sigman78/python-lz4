from pathlib import Path
import tomllib
from setuptools import Extension, setup

metadata = tomllib.loads(Path(__file__).with_name("pyproject.toml").read_text(encoding="utf-8"))
setup(ext_modules=[Extension(
    "lz4ext", ["src/python-lz4.c", "src/lz4.c", "src/lz4hc.c"],
    define_macros=[("LZ4EXT_VERSION", '"%s"' % metadata["project"]["version"])],
)])
