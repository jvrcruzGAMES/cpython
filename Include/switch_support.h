#ifndef Py_SWITCH_SUPPORT_H
#define Py_SWITCH_SUPPORT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_LIMITED_API
#  ifdef __SWITCH__
PyAPI_FUNC(int) PySwitch_EnableSysIntegration(void);
PyAPI_FUNC(int) PySwitch_DisableSysIntegration(void);
PyAPI_FUNC(int) PySwitch_IsSysIntegrationEnabled(void);
PyAPI_FUNC(int) PySwitch_EnableCurlIntegration(void);
PyAPI_FUNC(int) PySwitch_DisableCurlIntegration(void);
PyAPI_FUNC(int) PySwitch_IsCurlIntegrationEnabled(void);
PyAPI_FUNC(int) PySwitch_EnableSSLIntegration(void);
PyAPI_FUNC(int) PySwitch_DisableSSLIntegration(void);
PyAPI_FUNC(int) PySwitch_IsSSLIntegrationEnabled(void);
PyAPI_FUNC(int) PySwitch_EnableInputIntegration(void);
PyAPI_FUNC(int) PySwitch_DisableInputIntegration(void);
PyAPI_FUNC(int) PySwitch_IsInputIntegrationEnabled(void);
#  endif
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_SWITCH_SUPPORT_H */
