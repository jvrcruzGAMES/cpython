#include <Python.h>

static PyObject *
nx_native_demo_add(PyObject *self, PyObject *args)
{
    long left;
    long right;

    if (!PyArg_ParseTuple(args, "ll", &left, &right)) {
        return NULL;
    }
    return PyLong_FromLong(left + right);
}

static PyObject *
nx_native_demo_origin(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyUnicode_FromString("linked into the host NRO");
}

static PyMethodDef nx_native_demo_methods[] = {
    {"add", nx_native_demo_add, METH_VARARGS, NULL},
    {"origin", nx_native_demo_origin, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef nx_native_demo_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_nx_native_demo",
    .m_doc = "Native NX extension module linked into the host application.",
    .m_size = 0,
    .m_methods = nx_native_demo_methods,
};

PyMODINIT_FUNC
PyInit__nx_native_demo(void)
{
    return PyModule_Create(&nx_native_demo_module);
}
