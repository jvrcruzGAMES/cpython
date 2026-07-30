#include "Python.h"
#include "switch_support.h"
#include "structmember.h"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/version.h>

typedef struct {
    PyObject_HEAD
    int protocol;
    int verify_mode;
    int check_hostname;
    PyObject *cafile;
    PyObject *capath;
    PyObject *cadata;
    PyObject *ciphers;
}
SwitchSSLContext;

static PyTypeObject SwitchSSLContext_Type;
static PyObject *SwitchSSLError;

static int
switch_ssl_require_enabled(void)
{
    if (!PySwitch_IsSSLIntegrationEnabled()) {
        PyErr_SetString(PyExc_PermissionError,
                        "switch_ssl is disabled; enable it with "
                        "PySwitch_EnableSSLIntegration() from the host NRO");
        return -1;
    }
    return 0;
}

static void
SwitchSSLContext_dealloc(SwitchSSLContext *self)
{
    Py_CLEAR(self->cafile);
    Py_CLEAR(self->capath);
    Py_CLEAR(self->cadata);
    Py_CLEAR(self->ciphers);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
SwitchSSLContext_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    static char *keywords[] = {"protocol", NULL};
    int protocol = MBEDTLS_SSL_MINOR_VERSION_3;
    SwitchSSLContext *self;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i:SSLContext",
                                     keywords, &protocol)) {
        return NULL;
    }

    self = (SwitchSSLContext *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    self->protocol = protocol;
    self->verify_mode = 2;
    self->check_hostname = 1;
    self->cafile = Py_NewRef(Py_None);
    self->capath = Py_NewRef(Py_None);
    self->cadata = Py_NewRef(Py_None);
    self->ciphers = Py_NewRef(Py_None);
    return (PyObject *)self;
}

static PyObject *
SwitchSSLContext_load_verify_locations(SwitchSSLContext *self,
                                       PyObject *args, PyObject *kwds)
{
    static char *keywords[] = {"cafile", "capath", "cadata", NULL};
    PyObject *cafile = Py_None;
    PyObject *capath = Py_None;
    PyObject *cadata = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOO:load_verify_locations",
                                     keywords, &cafile, &capath, &cadata)) {
        return NULL;
    }
    Py_SETREF(self->cafile, Py_NewRef(cafile));
    Py_SETREF(self->capath, Py_NewRef(capath));
    Py_SETREF(self->cadata, Py_NewRef(cadata));
    Py_RETURN_NONE;
}

static PyObject *
SwitchSSLContext_set_ciphers(SwitchSSLContext *self, PyObject *arg)
{
    Py_SETREF(self->ciphers, Py_NewRef(arg));
    Py_RETURN_NONE;
}

static PyObject *
SwitchSSLContext_get_ciphers(SwitchSSLContext *self, PyObject *Py_UNUSED(ignored))
{
    if (self->ciphers == Py_None) {
        return PyList_New(0);
    }
    return Py_BuildValue("[{s:O}]", "name", self->ciphers);
}

static PyObject *
SwitchSSLContext_wrap_socket(SwitchSSLContext *self, PyObject *args,
                             PyObject *kwds)
{
    PyObject *sock;
    PyObject *server_hostname = Py_None;
    static char *keywords[] = {"sock", "server_side", "server_hostname", NULL};
    int server_side = 0;

    if (switch_ssl_require_enabled() < 0) {
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|pO:wrap_socket",
                                     keywords, &sock, &server_side,
                                     &server_hostname)) {
        return NULL;
    }

    PyErr_SetString(SwitchSSLError,
                    "mbedTLS socket wrapping is not implemented yet");
    return NULL;
}

static PyObject *
SwitchSSLContext_cert_store_stats(SwitchSSLContext *self, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("{s:i,s:i,s:i}", "x509", 0, "crl", 0, "x509_ca", 0);
}

static PyObject *
SwitchSSLContext_set_default_verify_paths(SwitchSSLContext *self,
                                          PyObject *Py_UNUSED(ignored))
{
    Py_RETURN_NONE;
}

static PyMethodDef SwitchSSLContext_methods[] = {
    {"load_verify_locations", _PyCFunction_CAST(SwitchSSLContext_load_verify_locations),
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_ciphers", (PyCFunction)SwitchSSLContext_set_ciphers, METH_O, NULL},
    {"get_ciphers", (PyCFunction)SwitchSSLContext_get_ciphers, METH_NOARGS, NULL},
    {"wrap_socket", _PyCFunction_CAST(SwitchSSLContext_wrap_socket),
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"cert_store_stats", (PyCFunction)SwitchSSLContext_cert_store_stats,
     METH_NOARGS, NULL},
    {"set_default_verify_paths",
     (PyCFunction)SwitchSSLContext_set_default_verify_paths, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}
};

static PyMemberDef SwitchSSLContext_members[] = {
    {"protocol", Py_T_INT, offsetof(SwitchSSLContext, protocol), 0, NULL},
    {"verify_mode", Py_T_INT, offsetof(SwitchSSLContext, verify_mode), 0, NULL},
    {"check_hostname", Py_T_INT, offsetof(SwitchSSLContext, check_hostname), 0, NULL},
    {"cafile", Py_T_OBJECT_EX, offsetof(SwitchSSLContext, cafile), 0, NULL},
    {"capath", Py_T_OBJECT_EX, offsetof(SwitchSSLContext, capath), 0, NULL},
    {"cadata", Py_T_OBJECT_EX, offsetof(SwitchSSLContext, cadata), 0, NULL},
    {NULL}
};

static PyTypeObject SwitchSSLContext_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "switch_ssl.SSLContext",
    .tp_basicsize = sizeof(SwitchSSLContext),
    .tp_dealloc = (destructor)SwitchSSLContext_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Switch mbedTLS SSL context configuration object.",
    .tp_methods = SwitchSSLContext_methods,
    .tp_members = SwitchSSLContext_members,
    .tp_new = SwitchSSLContext_new,
};

static PyObject *
switch_ssl_backends(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *module = PyImport_ImportModule("switch_support");
    PyObject *backends;

    if (module == NULL) {
        return NULL;
    }
    backends = PyObject_CallMethod(module, "backends", NULL);
    Py_DECREF(module);
    return backends;
}

static PyObject *
switch_ssl_mbedtls_version(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    char version[18];
    mbedtls_version_get_string_full(version);
    return PyUnicode_FromString(version);
}

static PyObject *
switch_ssl_rand_bytes(PyObject *self, PyObject *arg)
{
    Py_ssize_t size;
    PyObject *bytes;
    unsigned char *buffer;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const unsigned char personalization[] = "cpython-switch";
    int rc;

    if (switch_ssl_require_enabled() < 0) {
        return NULL;
    }
    size = PyLong_AsSsize_t(arg);
    if (size < 0) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError, "number of bytes must be non-negative");
        }
        return NULL;
    }

    bytes = PyBytes_FromStringAndSize(NULL, size);
    if (bytes == NULL) {
        return NULL;
    }
    buffer = (unsigned char *)PyBytes_AS_STRING(bytes);

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    rc = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                               personalization,
                               sizeof(personalization) - 1);
    if (rc == 0 && size > 0) {
        rc = mbedtls_ctr_drbg_random(&ctr_drbg, buffer, (size_t)size);
    }
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    if (rc != 0) {
        Py_DECREF(bytes);
        return PyErr_Format(PyExc_OSError,
                            "mbedTLS random generation failed: -0x%04x",
                            -rc);
    }
    return bytes;
}

static PyObject *
switch_ssl_rand_status(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    if (switch_ssl_require_enabled() < 0) {
        return NULL;
    }
    Py_RETURN_TRUE;
}

static PyObject *
switch_ssl_get_default_verify_paths(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("(OOOO)", Py_None, Py_None, Py_None, Py_None);
}

static PyObject *
switch_ssl_get_protocol_name(PyObject *self, PyObject *arg)
{
    int protocol = PyLong_AsInt(arg);
    if (protocol == -1 && PyErr_Occurred()) {
        return NULL;
    }
    switch (protocol) {
    case 16:
        return PyUnicode_FromString("PROTOCOL_TLS");
    case 17:
        return PyUnicode_FromString("PROTOCOL_TLS_SERVER");
    default:
        return PyUnicode_FromFormat("PROTOCOL_%d", protocol);
    }
}

static PyObject *
switch_ssl_enum_certificates(PyObject *self, PyObject *arg)
{
    return PyList_New(0);
}

static PyObject *
switch_ssl_get_server_certificate(PyObject *self, PyObject *args,
                                  PyObject *kwds)
{
    if (switch_ssl_require_enabled() < 0) {
        return NULL;
    }
    PyErr_SetString(SwitchSSLError,
                    "server certificate fetching is not implemented yet");
    return NULL;
}

static PyObject *
switch_ssl_wrap_socket(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (switch_ssl_require_enabled() < 0) {
        return NULL;
    }
    PyErr_SetString(SwitchSSLError,
                    "mbedTLS socket wrapping is not implemented yet");
    return NULL;
}

static PyObject *
switch_ssl_create_default_context(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *keywords[] = {"purpose", "cafile", "capath", "cadata", NULL};
    PyObject *purpose = Py_None;
    PyObject *cafile = Py_None;
    PyObject *capath = Py_None;
    PyObject *cadata = Py_None;
    PyObject *ctx;

    if (switch_ssl_require_enabled() < 0) {
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOOO:create_default_context",
                                     keywords, &purpose, &cafile, &capath,
                                     &cadata)) {
        return NULL;
    }
    ctx = PyObject_CallFunction((PyObject *)&SwitchSSLContext_Type, "i", 16);
    if (ctx == NULL) {
        return NULL;
    }
    if (cafile != Py_None || capath != Py_None || cadata != Py_None) {
        PyObject *result = PyObject_CallMethod(ctx, "load_verify_locations",
                                              "OOO", cafile, capath, cadata);
        if (result == NULL) {
            Py_DECREF(ctx);
            return NULL;
        }
        Py_DECREF(result);
    }
    return ctx;
}

static PyObject *
switch_ssl_mbedtls_strerror(PyObject *self, PyObject *arg)
{
    int rc;
    char buffer[256];

    rc = PyLong_AsInt(arg);
    if (rc == -1 && PyErr_Occurred()) {
        return NULL;
    }
    mbedtls_strerror(rc, buffer, sizeof(buffer));
    return PyUnicode_FromString(buffer);
}

PyDoc_STRVAR(switch_ssl_backends__doc__,
"backends($module, /)\n"
"--\n"
"\n"
"Return the Switch portlib backends compiled into this Python build.");

static PyMethodDef switch_ssl_methods[] = {
    {"backends", switch_ssl_backends, METH_NOARGS,
     switch_ssl_backends__doc__},
    {"mbedtls_version", switch_ssl_mbedtls_version, METH_NOARGS, NULL},
    {"RAND_bytes", switch_ssl_rand_bytes, METH_O, NULL},
    {"RAND_status", switch_ssl_rand_status, METH_NOARGS, NULL},
    {"get_default_verify_paths", switch_ssl_get_default_verify_paths,
     METH_NOARGS, NULL},
    {"get_protocol_name", switch_ssl_get_protocol_name, METH_O, NULL},
    {"enum_certificates", switch_ssl_enum_certificates, METH_O, NULL},
    {"get_server_certificate", _PyCFunction_CAST(switch_ssl_get_server_certificate),
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"wrap_socket", _PyCFunction_CAST(switch_ssl_wrap_socket),
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"create_default_context", _PyCFunction_CAST(switch_ssl_create_default_context),
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"mbedtls_strerror", switch_ssl_mbedtls_strerror, METH_O, NULL},
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
    PyObject *module;
    PyObject *version_info;
    char version[18];

    if (PyType_Ready(&SwitchSSLContext_Type) < 0) {
        return NULL;
    }

    module = PyModule_Create(&switch_ssl_module);
    if (module == NULL) {
        return NULL;
    }

    mbedtls_version_get_string_full(version);
    SwitchSSLError = PyErr_NewException("switch_ssl.SSLError", PyExc_OSError, NULL);
    if (SwitchSSLError == NULL) {
        Py_DECREF(module);
        return NULL;
    }
    if (PyModule_AddObjectRef(module, "SSLError", SwitchSSLError) < 0) {
        Py_DECREF(SwitchSSLError);
        Py_DECREF(module);
        return NULL;
    }

    if (PyModule_AddStringConstant(module, "OPENSSL_VERSION", version) < 0 ||
        PyModule_AddIntConstant(module, "OPENSSL_VERSION_NUMBER",
                                MBEDTLS_VERSION_NUMBER) < 0 ||
        PyModule_AddIntConstant(module, "CERT_NONE", 0) < 0 ||
        PyModule_AddIntConstant(module, "CERT_OPTIONAL", 1) < 0 ||
        PyModule_AddIntConstant(module, "CERT_REQUIRED", 2) < 0 ||
        PyModule_AddIntConstant(module, "PROTOCOL_TLS", 16) < 0 ||
        PyModule_AddIntConstant(module, "PROTOCOL_TLS_CLIENT", 16) < 0 ||
        PyModule_AddIntConstant(module, "PROTOCOL_TLS_SERVER", 17) < 0 ||
        PyModule_AddIntConstant(module, "HAS_SNI", 1) < 0 ||
        PyModule_AddIntConstant(module, "HAS_ALPN", 1) < 0) {
        Py_DECREF(SwitchSSLError);
        Py_DECREF(module);
        return NULL;
    }

    version_info = Py_BuildValue("(iiis)", MBEDTLS_VERSION_MAJOR,
                                 MBEDTLS_VERSION_MINOR,
                                 MBEDTLS_VERSION_PATCH,
                                 "mbedTLS");
    if (version_info == NULL) {
        Py_DECREF(SwitchSSLError);
        Py_DECREF(module);
        return NULL;
    }
    if (PyModule_AddObject(module, "OPENSSL_VERSION_INFO", version_info) < 0) {
        Py_DECREF(version_info);
        Py_DECREF(SwitchSSLError);
        Py_DECREF(module);
        return NULL;
    }

    Py_INCREF(&SwitchSSLContext_Type);
    if (PyModule_AddObject(module, "SSLContext",
                           (PyObject *)&SwitchSSLContext_Type) < 0) {
        Py_DECREF(&SwitchSSLContext_Type);
        Py_DECREF(SwitchSSLError);
        Py_DECREF(module);
        return NULL;
    }

    return module;
}
