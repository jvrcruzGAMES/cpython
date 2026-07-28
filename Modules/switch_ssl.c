#include "Python.h"

#include <lzma.h>
#include <mbedtls/version.h>
#include <zlib.h>
#include <zstd.h>

#ifdef HAVE_SWITCH_CURL
#  include <curl/curl.h>
#endif

static PyObject *
switch_ssl_backends(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *backends = PyDict_New();
    PyObject *compression;
    PyObject *tls;
    PyObject *http;
    char mbedtls_version[18];

    if (backends == NULL) {
        return NULL;
    }

    compression = Py_BuildValue(
        "{s:s,s:s,s:s}",
        "zlib", zlibVersion(),
        "lzma", lzma_version_string(),
        "zstd", ZSTD_versionString());
    if (compression == NULL) {
        Py_DECREF(backends);
        return NULL;
    }
    if (PyDict_SetItemString(backends, "compression", compression) < 0) {
        Py_DECREF(compression);
        Py_DECREF(backends);
        return NULL;
    }
    Py_DECREF(compression);

    mbedtls_version_get_string_full(mbedtls_version);
    tls = Py_BuildValue(
        "{s:s,s:O}",
        "mbedtls", mbedtls_version,
        "openssl_compat", Py_False);
    if (tls == NULL) {
        Py_DECREF(backends);
        return NULL;
    }
    if (PyDict_SetItemString(backends, "tls", tls) < 0) {
        Py_DECREF(tls);
        Py_DECREF(backends);
        return NULL;
    }
    Py_DECREF(tls);

#ifdef HAVE_SWITCH_CURL
    http = Py_BuildValue(
        "{s:O,s:s}",
        "switch_curl", Py_True,
        "version", curl_version());
#else
    http = Py_BuildValue(
        "{s:O,s:O}",
        "switch_curl", Py_False,
        "version", Py_None);
#endif
    if (http == NULL) {
        Py_DECREF(backends);
        return NULL;
    }
    if (PyDict_SetItemString(backends, "http", http) < 0) {
        Py_DECREF(http);
        Py_DECREF(backends);
        return NULL;
    }
    Py_DECREF(http);

    return backends;
}

PyDoc_STRVAR(switch_ssl_backends__doc__,
"backends($module, /)\n"
"--\n"
"\n"
"Return the Switch portlib backends compiled into this Python build.");

static PyMethodDef switch_ssl_methods[] = {
    {"backends", switch_ssl_backends, METH_NOARGS,
     switch_ssl_backends__doc__},
    {NULL, NULL, 0, NULL}
};

PyDoc_STRVAR(switch_ssl_doc,
"Nintendo Switch SSL/backend support helpers.");

static struct PyModuleDef switch_ssl_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "switch_ssl",
    .m_doc = switch_ssl_doc,
    .m_size = 0,
    .m_methods = switch_ssl_methods,
};

PyMODINIT_FUNC
PyInit_switch_ssl(void)
{
    return PyModule_Create(&switch_ssl_module);
}
