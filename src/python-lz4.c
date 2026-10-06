/*
 * Copyright (c) 2012-2013, Steeve Morin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Steeve Morin nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdlib.h>
#include <stdint.h>
#include "lz4.h"
#include "lz4hc.h"
#include "python-lz4.h"

typedef int (*compressor)(const char *source, char *dest, int isize, int capacity);

static int compress_hc(const char *source, char *dest, int isize, int capacity) {
    return LZ4_compress_HC(source, dest, isize, capacity, LZ4HC_CLEVEL_DEFAULT);
}

static int compression_bound(Py_ssize_t source_size) {
    int bound;
    if (source_size > LZ4_MAX_INPUT_SIZE) {
        PyErr_SetString(PyExc_OverflowError, "input exceeds LZ4 maximum input size");
        return 0;
    }
    bound = LZ4_compressBound((int)source_size);
    if (bound <= 0 || bound > INT_MAX - (int)sizeof(uint32_t)) {
        PyErr_SetString(PyExc_OverflowError, "compressed allocation size exceeds integer limit");
        return 0;
    }
    return bound;
}

static inline void store_le32(char *c, uint32_t x) {
    c[0] = x & 0xff;
    c[1] = (x >> 8) & 0xff;
    c[2] = (x >> 16) & 0xff;
    c[3] = (x >> 24) & 0xff;
}

static inline uint32_t load_le32(const char *c) {
    const uint8_t *d = (const uint8_t *)c;
    return (uint32_t)d[0] | ((uint32_t)d[1] << 8) |
           ((uint32_t)d[2] << 16) | ((uint32_t)d[3] << 24);
}

static const int hdr_size = sizeof(uint32_t);
#define DEFAULT_MAX_OUTPUT_SIZE (64 * 1024 * 1024)
#ifndef LZ4EXT_VERSION
#define LZ4EXT_VERSION "1.0"
#endif

static int validate_output_limit(Py_ssize_t limit) {
    if (limit <= 0 || limit > INT_MAX) {
        PyErr_SetString(PyExc_ValueError, "max_output_size must be between 1 and INT_MAX");
        return 0;
    }
    return 1;
}

static PyObject *compress_with(compressor compress, const char *source, Py_ssize_t source_size) {
    PyObject *result;
    char *dest;
    int dest_size;
    int capacity;

    capacity = compression_bound(source_size);
    if (capacity == 0) {
        return NULL;
    }

    dest_size = hdr_size + capacity;
    result = PyBytes_FromStringAndSize(NULL, dest_size);
    if (result == NULL) {
        return NULL;
    }
    dest = PyBytes_AS_STRING(result);
    store_le32(dest, (uint32_t)source_size);
    {
        int osize = compress(source, dest + hdr_size, (int)source_size, capacity);
        int actual_size = hdr_size + osize;
        if (osize <= 0) {
            Py_DECREF(result);
            PyErr_SetString(PyExc_ValueError, "compression failed");
            return NULL;
        }
        if (_PyBytes_Resize(&result, actual_size) < 0)
            return NULL;
    }
    return result;
}

static PyObject *parse_compression(compressor compress, PyObject *args) {
    Py_buffer input;
    PyObject *result;
    if (!PyArg_ParseTuple(args, "y*", &input))
        return NULL;
    result = compress_with(compress, input.buf, input.len);
    PyBuffer_Release(&input);
    return result;
}

static PyObject *py_lz4_compress(PyObject *self, PyObject *args) {
    return parse_compression(LZ4_compress_default, args);
}

static PyObject *py_lz4_compressHC(PyObject *self, PyObject *args) {
    return parse_compression(compress_hc, args);
}

static PyObject *decompress_prefixed(const char *source, Py_ssize_t source_size, Py_ssize_t max_output_size) {
    PyObject *result;
    uint32_t dest_size;
    if (!validate_output_limit(max_output_size)) {
        return NULL;
    }

    if (source_size > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "input exceeds LZ4 integer size limit");
        return NULL;
    }

    if (source_size < hdr_size) {
        PyErr_SetString(PyExc_ValueError, "input too short");
        return NULL;
    }
    dest_size = load_le32(source);
    if (dest_size > INT_MAX) {
        PyErr_Format(PyExc_ValueError, "invalid size in header: 0x%x", dest_size);
        return NULL;
    }
    if (dest_size > (uint32_t)max_output_size) {
        PyErr_SetString(PyExc_ValueError, "declared output exceeds max_output_size");
        return NULL;
    }
    if (dest_size == 0) {
        if (source_size == hdr_size ||
            (source_size == hdr_size + 1 && source[hdr_size] == 0)) {
            return PyBytes_FromStringAndSize("", 0);
        }
        PyErr_SetString(PyExc_ValueError, "invalid empty block");
        return NULL;
    }
    result = PyBytes_FromStringAndSize(NULL, dest_size);
    if (result != NULL && dest_size > 0) {
        char *dest = PyBytes_AS_STRING(result);
        int osize = LZ4_decompress_safe(source + hdr_size, dest, (int)source_size - hdr_size, dest_size);
        if (osize < 0) {
            PyErr_SetString(PyExc_ValueError, "invalid compressed block");
            Py_CLEAR(result);
        } else if ((uint32_t)osize != dest_size) {
            PyErr_SetString(PyExc_ValueError, "decoded size does not match header");
            Py_CLEAR(result);
        }
    }

    return result;
}

// RAW interface
static PyObject *compress_raw(const char *source, Py_ssize_t source_size) {
    PyObject *result;
    char *dest;
    int dest_size;

    dest_size = compression_bound(source_size);
    if (dest_size == 0) {
        return NULL;
    }

    result = PyBytes_FromStringAndSize(NULL, dest_size);
    if (result == NULL) {
        return NULL;
    }
    dest = PyBytes_AS_STRING(result);
    {
        int actual_size = LZ4_compress_default(source, dest, (int)source_size, dest_size);
        if (actual_size <= 0) {
            Py_DECREF(result);
            PyErr_SetString(PyExc_ValueError, "compression failed");
            return NULL;
        }
        if (_PyBytes_Resize(&result, actual_size) < 0)
            return NULL;
    }
    return result;    
}

static PyObject *decompress_raw(const char *source, Py_ssize_t source_size, int dest_size, Py_ssize_t max_output_size) {
    PyObject *result;
    int actual_size;

    if (!validate_output_limit(max_output_size)) {
        return NULL;
    }

    if (source_size > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "input exceeds LZ4 integer size limit");
        return NULL;
    }

    if (source_size == 0) {
        PyErr_SetString(PyExc_ValueError, "empty input is not an LZ4 block");
        return NULL;
    }
    if (dest_size < 0) {
        PyErr_SetString(PyExc_ValueError, "output capacity must be nonnegative");
        return NULL;
    }

    /* Retain the historical guess; callers may supply a larger capacity. */
    if (dest_size == 0) {
        if (source_size > INT_MAX / 2) {
            PyErr_SetString(PyExc_ValueError, "default output capacity exceeds LZ4 integer size limit");
            return NULL;
        }
        dest_size = 2 * (int)source_size;
    }
    if (dest_size > max_output_size) {
        PyErr_SetString(PyExc_ValueError, "output capacity exceeds max_output_size");
        return NULL;
    }

    result = PyBytes_FromStringAndSize(NULL, dest_size);
    if (result != NULL && dest_size > 0) {
        char *dest = PyBytes_AS_STRING(result);
        int osize = LZ4_decompress_safe(source, dest, (int)source_size, dest_size);
        if (osize < 0) {
            PyErr_SetString(PyExc_ValueError, "invalid block or insufficient output capacity");
            Py_DECREF(result);
            return NULL;
        }
        actual_size = osize;
        if (_PyBytes_Resize(&result, actual_size) < 0)
            return NULL;
    }

    return result;
}


static PyObject *py_lz4_compress_raw(PyObject *self, PyObject *args) {
    Py_buffer input;
    PyObject *result;
    if (!PyArg_ParseTuple(args, "y*", &input))
        return NULL;
    result = compress_raw(input.buf, input.len);
    PyBuffer_Release(&input);
    return result;
}

static PyObject *py_lz4_uncompress(PyObject *self, PyObject *args, PyObject *kwargs) {
    Py_buffer input;
    PyObject *result;
    Py_ssize_t max_output_size = DEFAULT_MAX_OUTPUT_SIZE;
    static char *keywords[] = {"data", "max_output_size", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y*|$n", keywords,
                                    &input, &max_output_size))
        return NULL;
    result = decompress_prefixed(input.buf, input.len, max_output_size);
    PyBuffer_Release(&input);
    return result;
}

static PyObject *py_lz4_uncompress_raw(PyObject *self, PyObject *args, PyObject *kwargs) {
    Py_buffer input;
    PyObject *result;
    int output_size = 0;
    Py_ssize_t max_output_size = DEFAULT_MAX_OUTPUT_SIZE;
    static char *keywords[] = {"data", "output_size", "max_output_size", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y*|i$n", keywords,
                                    &input, &output_size, &max_output_size))
        return NULL;
    result = decompress_raw(input.buf, input.len, output_size, max_output_size);
    PyBuffer_Release(&input);
    return result;
}

static PyMethodDef Lz4Methods[] = {
    {"LZ4_compress",  py_lz4_compress, METH_VARARGS, COMPRESS_DOCSTRING},
    {"LZ4_uncompress",  (PyCFunction)py_lz4_uncompress, METH_VARARGS | METH_KEYWORDS, UNCOMPRESS_DOCSTRING},
    {"compress",  py_lz4_compress, METH_VARARGS, COMPRESS_DOCSTRING},
    {"compressHC",  py_lz4_compressHC, METH_VARARGS, COMPRESSHC_DOCSTRING},
    {"uncompress",  (PyCFunction)py_lz4_uncompress, METH_VARARGS | METH_KEYWORDS, UNCOMPRESS_DOCSTRING},
    {"decompress",  (PyCFunction)py_lz4_uncompress, METH_VARARGS | METH_KEYWORDS, UNCOMPRESS_DOCSTRING},
    {"dumps",  py_lz4_compress, METH_VARARGS, COMPRESS_DOCSTRING},
    {"loads",  (PyCFunction)py_lz4_uncompress, METH_VARARGS | METH_KEYWORDS, UNCOMPRESS_DOCSTRING},
    {"compress_raw",  py_lz4_compress_raw, METH_VARARGS, COMPRESS_RAW_DOCSTRING},
    {"decompress_raw",  (PyCFunction)py_lz4_uncompress_raw, METH_VARARGS | METH_KEYWORDS, UNCOMPRESS_RAW_DOCSTRING},
    {"uncompress_raw",  (PyCFunction)py_lz4_uncompress_raw, METH_VARARGS | METH_KEYWORDS, UNCOMPRESS_RAW_DOCSTRING},
    {NULL, NULL, 0, NULL}
};



static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "lz4ext",
    NULL,
    0,
    Lz4Methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit_lz4ext(void) {
    PyObject *module = PyModule_Create(&moduledef);
    if (module == NULL)
        return NULL;
    if (PyModule_AddStringConstant(module, "__version__", LZ4EXT_VERSION) < 0 ||
        PyModule_AddStringConstant(module, "VERSION", LZ4EXT_VERSION) < 0 ||
        PyModule_AddStringConstant(module, "LZ4_VERSION", LZ4_versionString()) < 0 ||
        PyModule_AddIntConstant(module, "DEFAULT_MAX_OUTPUT_SIZE", DEFAULT_MAX_OUTPUT_SIZE) < 0) {
        Py_DECREF(module);
        return NULL;
    }
    return module;
}
