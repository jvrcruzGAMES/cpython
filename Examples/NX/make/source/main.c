#include <Python.h>
#include <switch_support.h>
#include <switch.h>

#ifndef SWITCH_PYTHON_SITE_PACKAGES
#define SWITCH_PYTHON_SITE_PACKAGES "romfs:/python_site"
#endif

PyMODINIT_FUNC PyInit__nx_native_demo(void);

__attribute__((used)) size_t __stacksize__ = 32 * 1024 * 1024;

static PyStatus
append_bytes_path(PyWideStringList *paths, const char *path)
{
    size_t len;
    wchar_t *wide = Py_DecodeLocale(path, &len);
    if (wide == NULL) {
        return PyStatus_NoMemory();
    }

    PyStatus status = PyWideStringList_Append(paths, wide);
    PyMem_RawFree(wide);
    return status;
}

static int
print_python_status(PyStatus status)
{
    if (PyStatus_Exception(status)) {
        if (status.err_msg != NULL) {
            printf("Python error: %s\n", status.err_msg);
        }
        return 1;
    }
    return 0;
}

static void
print_python_exception_to_console(void)
{
    PyObject *type = NULL;
    PyObject *value = NULL;
    PyObject *traceback = NULL;

    PyErr_Fetch(&type, &value, &traceback);
    PyErr_NormalizeException(&type, &value, &traceback);

    if (type == NULL) {
        printf("Python execution failed without an exception\n");
        consoleUpdate(NULL);
        return;
    }

    PyObject *type_name = PyObject_GetAttrString(type, "__name__");
    PyObject *value_text = value != NULL ? PyObject_Str(value) : NULL;
    const char *type_utf8 = type_name != NULL ? PyUnicode_AsUTF8(type_name) : NULL;
    const char *value_utf8 = value_text != NULL ? PyUnicode_AsUTF8(value_text) : NULL;

    printf("Python exception: %s: %s\n",
           type_utf8 != NULL ? type_utf8 : "<unknown>",
           value_utf8 != NULL ? value_utf8 : "");
    consoleUpdate(NULL);

    Py_XDECREF(type_name);
    Py_XDECREF(value_text);
    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(traceback);
}

int
main(int argc, char *argv[])
{
    bool romfs_initialized = false;

    consoleInit(NULL);
    consoleUpdate(NULL);

    Result rc = romfsInit();
    if (R_FAILED(rc)) {
        printf("romfsInit failed: 0x%x\n", rc);
        printf("Press PLUS to exit.\n");
        goto wait_for_exit;
    }
    romfs_initialized = true;

    PyStatus status;
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);

    config.isolated = 1;
    config.site_import = 0;
    config.install_signal_handlers = 0;
    config.configure_c_stdio = 1;
    config.buffered_stdio = 0;
    config.write_bytecode = 0;
    config.use_hash_seed = 1;
    config.hash_seed = 0;

    if (PyImport_AppendInittab("_nx_native_demo", PyInit__nx_native_demo) < 0) {
        printf("failed to register _nx_native_demo\n");
        goto wait_for_exit;
    }

    printf("Initializing Python...\n");
    consoleUpdate(NULL);

    printf("Config argv...\n");
    consoleUpdate(NULL);
    status = PyConfig_SetBytesArgv(&config, argc, argv);
    if (print_python_status(status)) {
        PyConfig_Clear(&config);
        goto wait_for_exit;
    }

    printf("Config program...\n");
    consoleUpdate(NULL);
    status = PyConfig_SetBytesString(&config, &config.program_name, "python_switch_features");
    if (print_python_status(status)) {
        PyConfig_Clear(&config);
        goto wait_for_exit;
    }

    printf("Config home...\n");
    consoleUpdate(NULL);
    status = PyConfig_SetBytesString(&config, &config.home, "romfs:/python");
    if (print_python_status(status)) {
        PyConfig_Clear(&config);
        goto wait_for_exit;
    }

    printf("Config paths...\n");
    consoleUpdate(NULL);
    config.module_search_paths_set = 1;
    status = append_bytes_path(&config.module_search_paths, "romfs:/python/lib/python3.14");
    if (print_python_status(status)) {
        PyConfig_Clear(&config);
        goto wait_for_exit;
    }
    status = append_bytes_path(&config.module_search_paths, "romfs:/python_examples");
    if (print_python_status(status)) {
        PyConfig_Clear(&config);
        goto wait_for_exit;
    }
    status = append_bytes_path(&config.module_search_paths, SWITCH_PYTHON_SITE_PACKAGES);
    if (print_python_status(status)) {
        PyConfig_Clear(&config);
        goto wait_for_exit;
    }

    printf("Enter Py_InitializeFromConfig...\n");
    consoleUpdate(NULL);
    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (print_python_status(status)) {
        goto wait_for_exit;
    }

    printf("Python initialized. [NX example build marker: stdout-diagnose-2]\n");
    consoleUpdate(NULL);

    if (PySwitch_EnableSysIntegration() < 0) {
        print_python_exception_to_console();
        Py_Finalize();
        goto wait_for_exit;
    }
    if (PySwitch_EnableSSLIntegration() < 0) {
        print_python_exception_to_console();
        Py_Finalize();
        goto wait_for_exit;
    }
    if (PySwitch_EnableCurlIntegration() < 0) {
        print_python_exception_to_console();
        Py_Finalize();
        goto wait_for_exit;
    }
    if (PySwitch_EnableInputIntegration() < 0) {
        print_python_exception_to_console();
        Py_Finalize();
        goto wait_for_exit;
    }

    printf("Running Python script...\n");
    consoleUpdate(NULL);

    int result = PyRun_SimpleString(
        "import runpy, sys, traceback\n"
        "print('Running Python feature script...', flush=True)\n"
        "try:\n"
        "    runpy.run_path('romfs:/python_examples/demo.py', run_name='__main__')\n"
        "except BaseException:\n"
        "    traceback.print_exc(file=sys.stdout)\n"
        "    sys.stdout.flush()\n"
        "    raise\n"
        "print('Python feature script finished.', flush=True)\n"
    );
    bool python_failed = result != 0;
    if (python_failed) {
        print_python_exception_to_console();
    }
    else {
        printf("Python script returned successfully.\n");
    }
    consoleUpdate(NULL);

    Py_Finalize();
    if (!python_failed) {
        if (romfs_initialized) {
            romfsExit();
        }
        consoleExit(NULL);
        return 0;
    }

wait_for_exit:
    printf("\\nPress PLUS to exit.\\n");

    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        if (padGetButtonsDown(&pad) & HidNpadButton_Plus) {
            break;
        }
        consoleUpdate(NULL);
    }

    if (romfs_initialized) {
        romfsExit();
    }
    consoleExit(NULL);
    return 0;
}
