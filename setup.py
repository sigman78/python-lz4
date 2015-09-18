#!/usr/bin/env python


from setuptools import setup, find_packages, Extension
from setuptools.command.build_ext import build_ext

VERSION = (0, 7, 2)
VERSION_STR = ".".join([str(x) for x in VERSION])
LZ4_VER = "r119"

COPT =  {'msvc': ['/Ox', '/DVERSION=\"\\\"%s\\\"\"' % VERSION_STR, '/DLZ4_VERSION=\"\\\"%s\\\"\"' % LZ4_VER],
     'mingw32' : ['-O3', '-march=native', '-DVERSION="%s"' % VERSION_STR, '-DLZ4_VERSION="%s"' % LZ4_VER],
     'clang' : ['-O3', '-march=native', '-DVERSION="%s"' % VERSION_STR, '-DLZ4_VERSION="%s"' % LZ4_VER],
     'gcc' : ['-O3', '-march=native', '-DVERSION="%s"' % VERSION_STR, '-DLZ4_VERSION="%s"' % LZ4_VER]}

class build_ext_subclass( build_ext ):
    def build_extensions(self):
        c = self.compiler.compiler_type
        if c in COPT:
           for e in self.extensions:
               e.extra_compile_args = COPT[c]
        build_ext.build_extensions(self)

setup(
    name='lz4ext',
    version=VERSION_STR,
    description="LZ4 Bindings for Python",
    long_description=open('README.rst', 'r').read(),
    author='Steeve Morin, Anton Stuk',
    author_email='steeve.morin@gmail.com, sigman@ioupg.com',
    url='https://github.com/sigman78/python-lz4',
    packages=find_packages('src'),
    package_dir={'': 'src'},
    ext_modules=[
        Extension('lz4ext', [
            'src/lz4.c',
            'src/lz4hc.c',
            'src/python-lz4.c'
        ])
    ],
    setup_requires=["nose>=1.0"],
    test_suite = "nose.collector",
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'License :: OSI Approved :: BSD License',
        'Intended Audience :: Developers',
        'Programming Language :: C',
        'Programming Language :: Python',
        'Programming Language :: Python :: 2.6',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.3',
    ],
    cmdclass = {'build_ext': build_ext_subclass },
)
