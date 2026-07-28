#include <Python.h>
#include <switch.h>

int
main(int argc, char *argv[])
{
    consoleInit(NULL);

    PyStatus status;
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    status = PyConfig_SetBytesArgv(&config, argc, argv);
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(&config);
        consoleExit(NULL);
        return 1;
    }

    status = PyConfig_SetString(&config, &config.program_name, L"python");
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(&config);
        consoleExit(NULL);
        return 1;
    }

    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
        consoleExit(NULL);
        return 1;
    }

    PyRun_SimpleString(
        "import sys\n"
        "print('CPython on Nintendo Switch')\n"
        "print(sys.version)\n"
        "print('\\n'.join(sys.path))\n"
    );

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

    Py_Finalize();
    consoleExit(NULL);
    return 0;
}
