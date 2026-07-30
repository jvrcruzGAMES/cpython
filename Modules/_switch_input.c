#include "Python.h"
#include "switch_support.h"

#include <switch/applets/swkbd.h>
#include <switch/runtime/pad.h>
#include <switch/services/hid.h>

static PadState default_pad;
static int default_pad_initialized = 0;

typedef struct {
    const char *name;
    u64 mask;
} ButtonName;

static const ButtonName button_names[] = {
    {"A", HidNpadButton_A},
    {"B", HidNpadButton_B},
    {"X", HidNpadButton_X},
    {"Y", HidNpadButton_Y},
    {"STICK_L", HidNpadButton_StickL},
    {"STICK_R", HidNpadButton_StickR},
    {"L", HidNpadButton_L},
    {"R", HidNpadButton_R},
    {"ZL", HidNpadButton_ZL},
    {"ZR", HidNpadButton_ZR},
    {"PLUS", HidNpadButton_Plus},
    {"MINUS", HidNpadButton_Minus},
    {"LEFT", HidNpadButton_Left},
    {"UP", HidNpadButton_Up},
    {"RIGHT", HidNpadButton_Right},
    {"DOWN", HidNpadButton_Down},
    {"STICK_L_LEFT", HidNpadButton_StickLLeft},
    {"STICK_L_UP", HidNpadButton_StickLUp},
    {"STICK_L_RIGHT", HidNpadButton_StickLRight},
    {"STICK_L_DOWN", HidNpadButton_StickLDown},
    {"STICK_R_LEFT", HidNpadButton_StickRLeft},
    {"STICK_R_UP", HidNpadButton_StickRUp},
    {"STICK_R_RIGHT", HidNpadButton_StickRRight},
    {"STICK_R_DOWN", HidNpadButton_StickRDown},
    {"LEFT_SL", HidNpadButton_LeftSL},
    {"LEFT_SR", HidNpadButton_LeftSR},
    {"RIGHT_SL", HidNpadButton_RightSL},
    {"RIGHT_SR", HidNpadButton_RightSR},
    {NULL, 0}
};

static void
ensure_default_pad(void)
{
    if (!default_pad_initialized) {
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&default_pad);
        default_pad_initialized = 1;
    }
}

static int
switch_input_require_enabled(void)
{
    if (!PySwitch_IsInputIntegrationEnabled()) {
        PyErr_SetString(PyExc_PermissionError,
                        "switch_input is disabled; enable it with "
                        "PySwitch_EnableInputIntegration() from the host NRO");
        return -1;
    }
    return 0;
}

static PyObject *
button_names_from_mask(u64 mask)
{
    PyObject *list = PyList_New(0);

    if (list == NULL) {
        return NULL;
    }
    for (const ButtonName *button = button_names; button->name != NULL; button++) {
        if (mask & button->mask) {
            PyObject *name = PyUnicode_FromString(button->name);
            if (name == NULL || PyList_Append(list, name) < 0) {
                Py_XDECREF(name);
                Py_DECREF(list);
                return NULL;
            }
            Py_DECREF(name);
        }
    }
    return list;
}

static PyObject *
stick_tuple(HidAnalogStickState stick)
{
    return Py_BuildValue("(i,i)", stick.x, stick.y);
}

static PyObject *
state_dict(u64 buttons, u64 down, u64 up)
{
    PyObject *state = NULL;
    PyObject *button_names = NULL;
    PyObject *down_names = NULL;
    PyObject *up_names = NULL;
    PyObject *left = NULL;
    PyObject *right = NULL;

    button_names = button_names_from_mask(buttons);
    down_names = button_names_from_mask(down);
    up_names = button_names_from_mask(up);
    left = stick_tuple(padGetStickPos(&default_pad, 0));
    right = stick_tuple(padGetStickPos(&default_pad, 1));
    if (button_names == NULL || down_names == NULL || up_names == NULL ||
        left == NULL || right == NULL)
    {
        goto done;
    }

    state = Py_BuildValue(
        "{s:K,s:K,s:K,s:O,s:O,s:O,s:O,s:O,s:O,s:O}",
        "buttons", buttons,
        "buttons_down", down,
        "buttons_up", up,
        "button_names", button_names,
        "button_down_names", down_names,
        "button_up_names", up_names,
        "left_stick", left,
        "right_stick", right,
        "connected", padIsConnected(&default_pad) ? Py_True : Py_False,
        "handheld", padIsHandheld(&default_pad) ? Py_True : Py_False);

done:
    Py_XDECREF(button_names);
    Py_XDECREF(down_names);
    Py_XDECREF(up_names);
    Py_XDECREF(left);
    Py_XDECREF(right);
    return state;
}

static PyObject *
switch_input_configure(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"max_players", "style_set", NULL};
    unsigned int max_players = 1;
    unsigned int style_set = HidNpadStyleSet_NpadStandard;

    if (switch_input_require_enabled() < 0) {
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|II:configure", keywords,
                                     &max_players, &style_set))
    {
        return NULL;
    }
    padConfigureInput(max_players, style_set);
    padInitializeDefault(&default_pad);
    default_pad_initialized = 1;
    Py_RETURN_NONE;
}

static PyObject *
switch_input_update(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    u64 buttons;
    u64 down;
    u64 up;

    if (switch_input_require_enabled() < 0) {
        return NULL;
    }
    ensure_default_pad();
    padUpdate(&default_pad);
    buttons = padGetButtons(&default_pad);
    down = padGetButtonsDown(&default_pad);
    up = padGetButtonsUp(&default_pad);
    return state_dict(buttons, down, up);
}

static PyObject *
switch_input_get_buttons(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    if (switch_input_require_enabled() < 0) {
        return NULL;
    }
    ensure_default_pad();
    padUpdate(&default_pad);
    return PyLong_FromUnsignedLongLong(padGetButtons(&default_pad));
}

static PyObject *
switch_input_get_buttons_down(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    if (switch_input_require_enabled() < 0) {
        return NULL;
    }
    ensure_default_pad();
    padUpdate(&default_pad);
    return PyLong_FromUnsignedLongLong(padGetButtonsDown(&default_pad));
}

static PyObject *
switch_input_get_buttons_up(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    if (switch_input_require_enabled() < 0) {
        return NULL;
    }
    ensure_default_pad();
    padUpdate(&default_pad);
    return PyLong_FromUnsignedLongLong(padGetButtonsUp(&default_pad));
}

static PyObject *
switch_input_is_connected(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    if (switch_input_require_enabled() < 0) {
        return NULL;
    }
    ensure_default_pad();
    padUpdate(&default_pad);
    if (padIsConnected(&default_pad)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *
switch_input_button_names(PyObject *self, PyObject *arg)
{
    unsigned long long mask = PyLong_AsUnsignedLongLong(arg);
    if (switch_input_require_enabled() < 0) {
        return NULL;
    }
    if (PyErr_Occurred()) {
        return NULL;
    }
    return button_names_from_mask((u64)mask);
}

static PyObject *
switch_input_prompt_keyboard(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"prompt", "keyboard_type", "initial_text",
                               "max_length", "password", NULL};
    const char *prompt = "";
    const char *initial_text = "";
    int keyboard_type = SwkbdType_QWERTY;
    Py_ssize_t max_length = 500;
    int password = 0;
    SwkbdConfig config;
    Result rc;
    char *buffer;
    PyObject *result;

    if (switch_input_require_enabled() < 0) {
        return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|sisnp:prompt_keyboard",
                                     keywords, &prompt, &keyboard_type,
                                     &initial_text, &max_length, &password))
    {
        return NULL;
    }
    if (keyboard_type < SwkbdType_Normal || keyboard_type > SwkbdType_Unknown9) {
        PyErr_SetString(PyExc_ValueError, "invalid keyboard_type");
        return NULL;
    }
    if (max_length < 1 || max_length > 4096) {
        PyErr_SetString(PyExc_ValueError, "max_length must be between 1 and 4096");
        return NULL;
    }

    buffer = PyMem_Calloc((size_t)max_length + 1, 1);
    if (buffer == NULL) {
        return PyErr_NoMemory();
    }

    rc = swkbdCreate(&config, 0);
    if (R_FAILED(rc)) {
        PyMem_Free(buffer);
        return PyErr_Format(PyExc_RuntimeError,
                            "swkbdCreate failed: 0x%08x", rc);
    }

    if (password) {
        swkbdConfigMakePresetPassword(&config);
    }
    else {
        swkbdConfigMakePresetDefault(&config);
    }
    swkbdConfigSetType(&config, (SwkbdType)keyboard_type);
    swkbdConfigSetStringLenMax(&config, (u32)max_length);
    swkbdConfigSetReturnButtonFlag(&config, 1);
    if (password) {
        swkbdConfigSetPasswordFlag(&config, 1);
    }
    if (prompt[0] != '\0') {
        swkbdConfigSetHeaderText(&config, prompt);
        swkbdConfigSetGuideText(&config, prompt);
    }
    if (initial_text[0] != '\0') {
        swkbdConfigSetInitialText(&config, initial_text);
    }

    rc = swkbdShow(&config, buffer, (size_t)max_length + 1);
    swkbdClose(&config);
    if (R_FAILED(rc)) {
        PyMem_Free(buffer);
        Py_RETURN_NONE;
    }

    result = PyUnicode_FromString(buffer);
    PyMem_Free(buffer);
    return result;
}

static PyObject *
switch_input_enabled(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    if (PySwitch_IsInputIntegrationEnabled()) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyMethodDef switch_input_methods[] = {
    {"enabled", switch_input_enabled, METH_NOARGS, NULL},
    {"configure", _PyCFunction_CAST(switch_input_configure),
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"update", switch_input_update, METH_NOARGS, NULL},
    {"get_buttons", switch_input_get_buttons, METH_NOARGS, NULL},
    {"get_buttons_down", switch_input_get_buttons_down, METH_NOARGS, NULL},
    {"get_buttons_up", switch_input_get_buttons_up, METH_NOARGS, NULL},
    {"is_connected", switch_input_is_connected, METH_NOARGS, NULL},
    {"button_names", switch_input_button_names, METH_O, NULL},
    {"prompt_keyboard", _PyCFunction_CAST(switch_input_prompt_keyboard),
     METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef switch_input_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_switch_input",
    .m_doc = "Nintendo Switch controller input backend.",
    .m_size = 0,
    .m_methods = switch_input_methods,
};

PyMODINIT_FUNC
PyInit__switch_input(void)
{
    PyObject *module = PyModule_Create(&switch_input_module);
    PyObject *buttons;

    if (module == NULL) {
        return NULL;
    }

    buttons = PyDict_New();
    if (buttons == NULL) {
        Py_DECREF(module);
        return NULL;
    }
    for (const ButtonName *button = button_names; button->name != NULL; button++) {
        PyObject *value = PyLong_FromUnsignedLongLong(button->mask);
        if (value == NULL || PyDict_SetItemString(buttons, button->name, value) < 0) {
            Py_XDECREF(value);
            Py_DECREF(buttons);
            Py_DECREF(module);
            return NULL;
        }
        Py_DECREF(value);
    }
    if (PyModule_AddObject(module, "BUTTONS", buttons) < 0) {
        Py_DECREF(buttons);
        Py_DECREF(module);
        return NULL;
    }
    if (PyModule_AddIntConstant(module, "STYLE_STANDARD",
                                HidNpadStyleSet_NpadStandard) < 0 ||
        PyModule_AddIntConstant(module, "STYLE_FULL",
                                HidNpadStyleSet_NpadFullCtrl) < 0 ||
        PyModule_AddIntConstant(module, "KEYBOARD_TYPE_NORMAL",
                                SwkbdType_Normal) < 0 ||
        PyModule_AddIntConstant(module, "KEYBOARD_TYPE_NUMPAD",
                                SwkbdType_NumPad) < 0 ||
        PyModule_AddIntConstant(module, "KEYBOARD_TYPE_QWERTY",
                                SwkbdType_QWERTY) < 0 ||
        PyModule_AddIntConstant(module, "KEYBOARD_TYPE_LATIN",
                                SwkbdType_Latin) < 0 ||
        PyModule_AddIntConstant(module, "KEYBOARD_TYPE_ZH_HANS",
                                SwkbdType_ZhHans) < 0 ||
        PyModule_AddIntConstant(module, "KEYBOARD_TYPE_ZH_HANT",
                                SwkbdType_ZhHant) < 0 ||
        PyModule_AddIntConstant(module, "KEYBOARD_TYPE_KOREAN",
                                SwkbdType_Korean) < 0 ||
        PyModule_AddIntConstant(module, "KEYBOARD_TYPE_ALL",
                                SwkbdType_All) < 0)
    {
        Py_DECREF(module);
        return NULL;
    }
    return module;
}
