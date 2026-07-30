#include "Python.h"
#include "switch_support.h"

#include <curl/curl.h>
#include <switch.h>
#include <string.h>

static int curl_initialized = 0;
static int socket_initialized = 0;

typedef struct {
    char *data;
    Py_ssize_t size;
} Buffer;

static size_t
write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    Buffer *buffer = (Buffer *)userdata;
    size_t total = size * nmemb;
    char *resized;

    if (total == 0) {
        return 0;
    }
    if (buffer->size > PY_SSIZE_T_MAX - (Py_ssize_t)total) {
        return 0;
    }
    resized = PyMem_Realloc(buffer->data, buffer->size + (Py_ssize_t)total);
    if (resized == NULL) {
        return 0;
    }
    buffer->data = resized;
    memcpy(buffer->data + buffer->size, ptr, total);
    buffer->size += (Py_ssize_t)total;
    return total;
}

static int
bytes_from_string(PyObject *value, PyObject **bytes)
{
    if (PyBytes_Check(value)) {
        Py_INCREF(value);
        *bytes = value;
        return 0;
    }
    if (PyUnicode_Check(value)) {
        *bytes = PyUnicode_AsUTF8String(value);
        return *bytes != NULL ? 0 : -1;
    }
    PyErr_SetString(PyExc_TypeError, "expected str or bytes");
    return -1;
}

static int
set_string_option(CURL *curl, CURLoption option, PyObject *value)
{
    PyObject *bytes = NULL;
    CURLcode rc;

    if (bytes_from_string(value, &bytes) < 0) {
        return -1;
    }
    rc = curl_easy_setopt(curl, option, PyBytes_AS_STRING(bytes));
    Py_DECREF(bytes);
    if (rc != CURLE_OK) {
        PyErr_SetString(PyExc_RuntimeError, curl_easy_strerror(rc));
        return -1;
    }
    return 0;
}

static int
set_long_option(CURL *curl, CURLoption option, long value)
{
    CURLcode rc = curl_easy_setopt(curl, option, value);
    if (rc != CURLE_OK) {
        PyErr_SetString(PyExc_RuntimeError, curl_easy_strerror(rc));
        return -1;
    }
    return 0;
}

static int
set_timeout_option(CURL *curl, CURLoption option, PyObject *value)
{
    double seconds = PyFloat_AsDouble(value);
    long milliseconds;

    if (PyErr_Occurred()) {
        return -1;
    }
    if (seconds < 0.0) {
        PyErr_SetString(PyExc_ValueError, "timeout must be non-negative");
        return -1;
    }
    milliseconds = (long)(seconds * 1000.0);
    return set_long_option(curl, option, milliseconds);
}

static int
ensure_socket_service(void)
{
    Result rc;

    if (socket_initialized) {
        return 0;
    }

    rc = socketInitializeDefault();
    if (R_FAILED(rc)) {
        PyErr_Format(PyExc_RuntimeError,
                     "socketInitializeDefault() failed: 0x%x", rc);
        return -1;
    }
    socket_initialized = 1;
    return 0;
}

static int
ensure_curl_runtime(void)
{
    if (ensure_socket_service() < 0) {
        return -1;
    }
    if (!curl_initialized) {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
            PyErr_SetString(PyExc_RuntimeError, "curl_global_init() failed");
            return -1;
        }
        curl_initialized = 1;
    }
    return 0;
}

static int
apply_options(CURL *curl, PyObject *options, struct curl_slist **headers,
              PyObject **postfields_owner)
{
    PyObject *value;

    value = PyDict_GetItemString(options, "URL");
    if (value != NULL && set_string_option(curl, CURLOPT_URL, value) < 0) {
        return -1;
    }

    value = PyDict_GetItemString(options, "CUSTOMREQUEST");
    if (value != NULL && set_string_option(curl, CURLOPT_CUSTOMREQUEST, value) < 0) {
        return -1;
    }

    value = PyDict_GetItemString(options, "USERAGENT");
    if (value != NULL && set_string_option(curl, CURLOPT_USERAGENT, value) < 0) {
        return -1;
    }

    value = PyDict_GetItemString(options, "ACCEPT_ENCODING");
    if (value != NULL && set_string_option(curl, CURLOPT_ACCEPT_ENCODING, value) < 0) {
        return -1;
    }

    value = PyDict_GetItemString(options, "CAINFO");
    if (value != NULL && set_string_option(curl, CURLOPT_CAINFO, value) < 0) {
        return -1;
    }

    value = PyDict_GetItemString(options, "FOLLOWLOCATION");
    if (value != NULL) {
        int truth = PyObject_IsTrue(value);
        if (truth < 0 ||
            set_long_option(curl, CURLOPT_FOLLOWLOCATION, truth ? 1L : 0L) < 0) {
            return -1;
        }
    }

    value = PyDict_GetItemString(options, "MAXREDIRS");
    if (value != NULL) {
        long maxredirs = PyLong_AsLong(value);
        if (PyErr_Occurred() ||
            set_long_option(curl, CURLOPT_MAXREDIRS, maxredirs) < 0) {
            return -1;
        }
    }

    value = PyDict_GetItemString(options, "SSL_VERIFYPEER");
    if (value != NULL) {
        int truth = PyObject_IsTrue(value);
        if (truth < 0 ||
            set_long_option(curl, CURLOPT_SSL_VERIFYPEER, truth ? 1L : 0L) < 0) {
            return -1;
        }
    }

    value = PyDict_GetItemString(options, "SSL_VERIFYHOST");
    if (value != NULL) {
        long verifyhost = PyLong_AsLong(value);
        if (PyErr_Occurred() ||
            set_long_option(curl, CURLOPT_SSL_VERIFYHOST, verifyhost) < 0) {
            return -1;
        }
    }

    value = PyDict_GetItemString(options, "TIMEOUT");
    if (value != NULL && set_timeout_option(curl, CURLOPT_TIMEOUT_MS, value) < 0) {
        return -1;
    }

    value = PyDict_GetItemString(options, "CONNECTTIMEOUT");
    if (value != NULL && set_timeout_option(curl, CURLOPT_CONNECTTIMEOUT_MS, value) < 0) {
        return -1;
    }

    value = PyDict_GetItemString(options, "HTTPGET");
    if (value != NULL) {
        int truth = PyObject_IsTrue(value);
        if (truth < 0 || (truth && set_long_option(curl, CURLOPT_HTTPGET, 1L) < 0)) {
            return -1;
        }
    }

    value = PyDict_GetItemString(options, "POST");
    if (value != NULL) {
        int truth = PyObject_IsTrue(value);
        if (truth < 0 || (truth && set_long_option(curl, CURLOPT_POST, 1L) < 0)) {
            return -1;
        }
    }

    value = PyDict_GetItemString(options, "POSTFIELDS");
    if (value != NULL) {
        char *data;
        Py_ssize_t size;

        if (PyUnicode_Check(value)) {
            value = PyUnicode_AsUTF8String(value);
            if (value == NULL) {
                return -1;
            }
            *postfields_owner = value;
        }
        else {
            Py_INCREF(value);
            *postfields_owner = value;
        }
        if (PyBytes_AsStringAndSize(*postfields_owner, &data, &size) < 0) {
            return -1;
        }
        if (curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data) != CURLE_OK ||
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE,
                             (curl_off_t)size) != CURLE_OK) {
            PyErr_SetString(PyExc_RuntimeError, "failed to set POSTFIELDS");
            return -1;
        }
    }

    value = PyDict_GetItemString(options, "HTTPHEADER");
    if (value != NULL) {
        PyObject *seq = PySequence_Fast(value, "HTTPHEADER must be a sequence");
        Py_ssize_t i, n;

        if (seq == NULL) {
            return -1;
        }
        n = PySequence_Fast_GET_SIZE(seq);
        for (i = 0; i < n; i++) {
            PyObject *item = PySequence_Fast_GET_ITEM(seq, i);
            PyObject *bytes = NULL;
            struct curl_slist *next;

            if (bytes_from_string(item, &bytes) < 0) {
                Py_DECREF(seq);
                return -1;
            }
            next = curl_slist_append(*headers, PyBytes_AS_STRING(bytes));
            Py_DECREF(bytes);
            if (next == NULL) {
                Py_DECREF(seq);
                PyErr_NoMemory();
                return -1;
            }
            *headers = next;
        }
        Py_DECREF(seq);
        if (*headers != NULL &&
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *headers) != CURLE_OK) {
            PyErr_SetString(PyExc_RuntimeError, "failed to set HTTPHEADER");
            return -1;
        }
    }

    return 0;
}

static PyObject *
switch_curl_perform(PyObject *self, PyObject *arg)
{
    CURL *curl;
    CURLcode rc;
    Buffer body = {NULL, 0};
    struct curl_slist *headers = NULL;
    PyObject *postfields_owner = NULL;
    PyObject *content = NULL;
    PyObject *result = NULL;
    char *effective_url = NULL;
    char *content_type = NULL;
    long response_code = 0;
    double total_time = 0.0;

    if (!PyDict_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "perform() expects an options dict");
        return NULL;
    }
    if (!PySwitch_IsCurlIntegrationEnabled()) {
        PyErr_SetString(PyExc_PermissionError,
                        "switch_curl is disabled; enable it with "
                        "PySwitch_EnableCurlIntegration() from the host NRO");
        return NULL;
    }
    if (ensure_curl_runtime() < 0) {
        return NULL;
    }

    curl = curl_easy_init();
    if (curl == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "curl_easy_init() failed");
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    if (apply_options(curl, arg, &headers, &postfields_owner) < 0) {
        goto done;
    }

    rc = curl_easy_perform(curl);

    if (rc != CURLE_OK) {
        PyErr_SetString(PyExc_RuntimeError, curl_easy_strerror(rc));
        goto done;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);

    content = PyBytes_FromStringAndSize(body.data, body.size);
    if (content == NULL) {
        goto done;
    }

    result = Py_BuildValue(
        "{s:s,s:l,s:O,s:{s:s},s:d}",
        "url", effective_url != NULL ? effective_url : "",
        "status_code", response_code,
        "content", content,
        "headers",
        "content-type", content_type != NULL ? content_type : "",
        "elapsed", total_time);

done:
    Py_XDECREF(content);
    Py_XDECREF(postfields_owner);
    PyMem_Free(body.data);
    if (headers != NULL) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
    return result;
}

static PyObject *
switch_curl_version(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyUnicode_FromString(curl_version());
}

static PyObject *
switch_curl_runtime_info(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue(
        "{s:O,s:O}",
        "socket_initialized", socket_initialized ? Py_True : Py_False,
        "curl_initialized", curl_initialized ? Py_True : Py_False);
}

static void
switch_curl_free(void *module)
{
    if (curl_initialized) {
        curl_global_cleanup();
        curl_initialized = 0;
    }
    if (socket_initialized) {
        socketExit();
        socket_initialized = 0;
    }
}

static PyMethodDef switch_curl_methods[] = {
    {"perform", switch_curl_perform, METH_O, NULL},
    {"version", switch_curl_version, METH_NOARGS, NULL},
    {"runtime_info", switch_curl_runtime_info, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef switch_curl_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_switch_curl",
    .m_doc = "Nintendo Switch curl backend.",
    .m_size = 0,
    .m_methods = switch_curl_methods,
    .m_free = switch_curl_free,
};

PyMODINIT_FUNC
PyInit__switch_curl(void)
{
    return PyModule_Create(&switch_curl_module);
}
