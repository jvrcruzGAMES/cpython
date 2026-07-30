#include "Python.h"
#include "switch_support.h"

#include <switch/kernel/svc.h>
#include <switch/result.h>
#include <switch/runtime/devices/console.h>
#include <switch/runtime/hosversion.h>
#include <switch/services/applet.h>
#include <switch/services/set.h>
#include <switch/types.h>

#include <lzma.h>
#include <mbedtls/version.h>
#include <zlib.h>
#include <zstd.h>
#include <string.h>

#ifdef HAVE_SWITCH_CURL
#  include <curl/curl.h>
#endif

static int switch_sys_integration_enabled = 0;
static int switch_curl_integration_enabled = 0;
static int switch_ssl_integration_enabled = 0;
static int switch_input_integration_enabled = 0;

static const char *
product_model_name(SetSysProductModel model)
{
    switch (model) {
    case SetSysProductModel_Nx:
        return "nx";
    case SetSysProductModel_Copper:
        return "copper";
    case SetSysProductModel_Iowa:
        return "iowa";
    case SetSysProductModel_Hoag:
        return "hoag";
    case SetSysProductModel_Calcio:
        return "calcio";
    case SetSysProductModel_Aula:
        return "aula";
    case SetSysProductModel_Invalid:
    default:
        return "invalid";
    }
}

static PyObject *
switch_support_is_switch(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    Py_RETURN_TRUE;
}

static PyObject *
switch_support_firmware_version(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    u32 version = hosversionGet();

    if (version == 0) {
        Py_RETURN_NONE;
    }
    return Py_BuildValue(
        "{s:i,s:i,s:i,s:I,s:O}",
        "major", HOSVER_MAJOR(version),
        "minor", HOSVER_MINOR(version),
        "micro", HOSVER_MICRO(version),
        "raw", version,
        "atmosphere", hosversionIsAtmosphere() ? Py_True : Py_False);
}

static PyObject *
switch_support_model(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    SetSysProductModel model;
    Result rc;

    rc = setsysInitialize();
    if (R_FAILED(rc)) {
        return Py_BuildValue("{s:O,s:I}", "name", Py_None, "result", rc);
    }
    rc = setsysGetProductModel(&model);
    setsysExit();
    if (R_FAILED(rc)) {
        return Py_BuildValue("{s:O,s:I}", "name", Py_None, "result", rc);
    }
    return Py_BuildValue(
        "{s:s,s:i,s:I}",
        "name", product_model_name(model),
        "id", (int)model,
        "result", rc);
}

static PyObject *
switch_support_backends(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *backends = PyDict_New();
    PyObject *compression;
    PyObject *tls;
    PyObject *http;
    PyObject *input;
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
        "{s:s,s:O,s:O}",
        "mbedtls", mbedtls_version,
        "openssl_compat", Py_False,
        "enabled", switch_ssl_integration_enabled ? Py_True : Py_False);
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
        "{s:O,s:s,s:O}",
        "switch_curl", Py_True,
        "version", curl_version(),
        "enabled", switch_curl_integration_enabled ? Py_True : Py_False);
#else
    http = Py_BuildValue(
        "{s:O,s:O,s:O}",
        "switch_curl", Py_False,
        "version", Py_None,
        "enabled", Py_False);
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

#ifdef HAVE_SWITCH_INPUT
    input = Py_BuildValue(
        "{s:O,s:O}",
        "switch_input", Py_True,
        "enabled", switch_input_integration_enabled ? Py_True : Py_False);
#else
    input = Py_BuildValue(
        "{s:O,s:O}",
        "switch_input", Py_False,
        "enabled", Py_False);
#endif
    if (input == NULL) {
        Py_DECREF(backends);
        return NULL;
    }
    if (PyDict_SetItemString(backends, "input", input) < 0) {
        Py_DECREF(input);
        Py_DECREF(backends);
        return NULL;
    }
    Py_DECREF(input);

    return backends;
}

static PyObject *
switch_support_environment(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *env = PyDict_New();
    PyObject *firmware;
    PyObject *model;
    PyObject *backends;

    if (env == NULL) {
        return NULL;
    }
    if (PyDict_SetItemString(env, "is_switch", Py_True) < 0) {
        Py_DECREF(env);
        return NULL;
    }

    firmware = switch_support_firmware_version(self, NULL);
    if (firmware == NULL) {
        Py_DECREF(env);
        return NULL;
    }
    if (PyDict_SetItemString(env, "firmware", firmware) < 0) {
        Py_DECREF(firmware);
        Py_DECREF(env);
        return NULL;
    }
    Py_DECREF(firmware);

    model = switch_support_model(self, NULL);
    if (model == NULL) {
        Py_DECREF(env);
        return NULL;
    }
    if (PyDict_SetItemString(env, "model", model) < 0) {
        Py_DECREF(model);
        Py_DECREF(env);
        return NULL;
    }
    Py_DECREF(model);

    backends = switch_support_backends(self, NULL);
    if (backends == NULL) {
        Py_DECREF(env);
        return NULL;
    }
    if (PyDict_SetItemString(env, "backends", backends) < 0) {
        Py_DECREF(backends);
        Py_DECREF(env);
        return NULL;
    }
    Py_DECREF(backends);

    return env;
}

static PyObject *
switch_support_sys_applet_main_loop(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    if (appletMainLoop()) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *
switch_support_sys_request_exit(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    Result rc = appletRequestExitToSelf();
    if (R_FAILED(rc)) {
        return Py_BuildValue("{s:O,s:I}", "ok", Py_False, "result", rc);
    }
    return Py_BuildValue("{s:O,s:I}", "ok", Py_True, "result", rc);
}

static PyObject *
switch_support_sys_console_update(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    consoleUpdate(NULL);
    Py_RETURN_NONE;
}

static PyObject *
switch_support_sys_sleep(PyObject *self, PyObject *arg)
{
    unsigned long long nanoseconds = PyLong_AsUnsignedLongLong(arg);
    if (PyErr_Occurred()) {
        return NULL;
    }
    svcSleepThread((s64)nanoseconds);
    Py_RETURN_NONE;
}

static PyObject *
switch_support_sys_is_switch(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    Py_RETURN_TRUE;
}

static PyMethodDef switch_support_sys_methods[] = {
    {"is_switch", switch_support_sys_is_switch, METH_NOARGS, NULL},
    {"applet_main_loop", switch_support_sys_applet_main_loop, METH_NOARGS, NULL},
    {"request_exit", switch_support_sys_request_exit, METH_NOARGS, NULL},
    {"console_update", switch_support_sys_console_update, METH_NOARGS, NULL},
    {"sleep", switch_support_sys_sleep, METH_O, NULL},
    {NULL, NULL, 0, NULL}
};

static int
switch_support_install_sys_integration(void)
{
    PyObject *sys_module;
    PyObject *switch_module;
    PyObject *firmware;
    PyObject *model;

    if (!Py_IsInitialized()) {
        return 0;
    }

    sys_module = PyImport_ImportModule("sys");
    if (sys_module == NULL) {
        return -1;
    }

    switch_module = PyModule_New("sys.switch");
    if (switch_module == NULL) {
        Py_DECREF(sys_module);
        return -1;
    }
    if (PyModule_AddFunctions(switch_module, switch_support_sys_methods) < 0) {
        Py_DECREF(switch_module);
        Py_DECREF(sys_module);
        return -1;
    }
    if (PyModule_AddObjectRef(switch_module, "enabled", Py_True) < 0) {
        Py_DECREF(switch_module);
        Py_DECREF(sys_module);
        return -1;
    }

    firmware = switch_support_firmware_version(NULL, NULL);
    if (firmware == NULL) {
        Py_DECREF(switch_module);
        Py_DECREF(sys_module);
        return -1;
    }
    if (PyModule_AddObject(switch_module, "firmware", firmware) < 0) {
        Py_DECREF(firmware);
        Py_DECREF(switch_module);
        Py_DECREF(sys_module);
        return -1;
    }

    model = switch_support_model(NULL, NULL);
    if (model == NULL) {
        Py_DECREF(switch_module);
        Py_DECREF(sys_module);
        return -1;
    }
    if (PyModule_AddObject(switch_module, "model", model) < 0) {
        Py_DECREF(model);
        Py_DECREF(switch_module);
        Py_DECREF(sys_module);
        return -1;
    }

    if (PyObject_SetAttrString(sys_module, "switch", switch_module) < 0) {
        Py_DECREF(switch_module);
        Py_DECREF(sys_module);
        return -1;
    }

    Py_DECREF(switch_module);
    Py_DECREF(sys_module);
    return 0;
}

int
PySwitch_EnableSysIntegration(void)
{
    switch_sys_integration_enabled = 1;
    return switch_support_install_sys_integration();
}

int
PySwitch_DisableSysIntegration(void)
{
    PyObject *sys_module;

    switch_sys_integration_enabled = 0;
    if (!Py_IsInitialized()) {
        return 0;
    }

    sys_module = PyImport_ImportModule("sys");
    if (sys_module == NULL) {
        return -1;
    }
    if (PyObject_HasAttrString(sys_module, "switch")) {
        if (PyObject_DelAttrString(sys_module, "switch") < 0) {
            Py_DECREF(sys_module);
            return -1;
        }
    }
    Py_DECREF(sys_module);
    return 0;
}

int
PySwitch_IsSysIntegrationEnabled(void)
{
    return switch_sys_integration_enabled;
}

int
PySwitch_EnableCurlIntegration(void)
{
    switch_curl_integration_enabled = 1;
    return 0;
}

int
PySwitch_DisableCurlIntegration(void)
{
    switch_curl_integration_enabled = 0;
    return 0;
}

int
PySwitch_IsCurlIntegrationEnabled(void)
{
    return switch_curl_integration_enabled;
}

int
PySwitch_EnableSSLIntegration(void)
{
    switch_ssl_integration_enabled = 1;
    return 0;
}

int
PySwitch_DisableSSLIntegration(void)
{
    switch_ssl_integration_enabled = 0;
    return 0;
}

int
PySwitch_IsSSLIntegrationEnabled(void)
{
    return switch_ssl_integration_enabled;
}

int
PySwitch_EnableInputIntegration(void)
{
    switch_input_integration_enabled = 1;
    return 0;
}

int
PySwitch_DisableInputIntegration(void)
{
    switch_input_integration_enabled = 0;
    return 0;
}

int
PySwitch_IsInputIntegrationEnabled(void)
{
    return switch_input_integration_enabled;
}

static PyObject *
switch_support_sys_integration_enabled(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    if (switch_sys_integration_enabled) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *
switch_support_curl_integration_enabled(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    if (switch_curl_integration_enabled) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *
switch_support_ssl_integration_enabled(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    if (switch_ssl_integration_enabled) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *
switch_support_switch_input_enabled(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    if (switch_input_integration_enabled) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *
switch_support_is_enabled(PyObject *self, PyObject *arg)
{
    const char *name = PyUnicode_AsUTF8(arg);

    if (name == NULL) {
        return NULL;
    }
    if (strcmp(name, "sys") == 0 || strcmp(name, "sys.switch") == 0) {
        if (switch_sys_integration_enabled) {
            Py_RETURN_TRUE;
        }
        Py_RETURN_FALSE;
    }
    if (strcmp(name, "ssl") == 0 || strcmp(name, "switch_ssl") == 0) {
        if (switch_ssl_integration_enabled) {
            Py_RETURN_TRUE;
        }
        Py_RETURN_FALSE;
    }
    if (strcmp(name, "curl") == 0 || strcmp(name, "switch_curl") == 0) {
        if (switch_curl_integration_enabled) {
            Py_RETURN_TRUE;
        }
        Py_RETURN_FALSE;
    }
    if (strcmp(name, "input") == 0 || strcmp(name, "switch_input") == 0) {
        if (switch_input_integration_enabled) {
            Py_RETURN_TRUE;
        }
        Py_RETURN_FALSE;
    }
    Py_RETURN_FALSE;
}

static PyMethodDef switch_support_methods[] = {
    {"is_switch", switch_support_is_switch, METH_NOARGS, NULL},
    {"firmware_version", switch_support_firmware_version, METH_NOARGS, NULL},
    {"model", switch_support_model, METH_NOARGS, NULL},
    {"backends", switch_support_backends, METH_NOARGS, NULL},
    {"environment", switch_support_environment, METH_NOARGS, NULL},
    {"sys_integration_enabled", switch_support_sys_integration_enabled, METH_NOARGS, NULL},
    {"is_sys_enabled", switch_support_sys_integration_enabled, METH_NOARGS, NULL},
    {"curl_integration_enabled", switch_support_curl_integration_enabled, METH_NOARGS, NULL},
    {"is_curl_enabled", switch_support_curl_integration_enabled, METH_NOARGS, NULL},
    {"ssl_integration_enabled", switch_support_ssl_integration_enabled, METH_NOARGS, NULL},
    {"is_ssl_enabled", switch_support_ssl_integration_enabled, METH_NOARGS, NULL},
    {"input_integration_enabled", switch_support_switch_input_enabled, METH_NOARGS, NULL},
    {"is_switch_input_enabled", switch_support_switch_input_enabled, METH_NOARGS, NULL},
    {"is_input_enabled", switch_support_switch_input_enabled, METH_NOARGS, NULL},
    {"is_enabled", switch_support_is_enabled, METH_O, NULL},
    {NULL, NULL, 0, NULL}
};

PyDoc_STRVAR(switch_support_doc,
"Nintendo Switch environment and backend support helpers.");

static struct PyModuleDef switch_support_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "switch_support",
    .m_doc = switch_support_doc,
    .m_size = 0,
    .m_methods = switch_support_methods,
};

PyMODINIT_FUNC
PyInit_switch_support(void)
{
    PyObject *module = PyModule_Create(&switch_support_module);
    if (module == NULL) {
        return NULL;
    }
    if (switch_sys_integration_enabled) {
        if (switch_support_install_sys_integration() < 0) {
            Py_DECREF(module);
            return NULL;
        }
    }
    return module;
}
