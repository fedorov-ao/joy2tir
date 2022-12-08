#include <cstdint>
#include <iostream>
#include <string>

struct sig_data {
  char dllsig[200];
  char appsig[200];
};

struct tir_data {
  short status;
  short frame;
  unsigned int checksum;
  float roll, pitch, yaw;
  float tx, ty, tz;
  float padding[9];
};

extern "C" int __declspec(dllexport) __stdcall NP_GetSignature(struct sig_data *sig);
extern "C" int __declspec(dllexport) __stdcall NP_QueryVersion(short *ver);
extern "C" int __declspec(dllexport) __stdcall NP_ReCenter();
extern "C" int __declspec(dllexport) __stdcall NP_RegisterWindowHandle(void *handle);
extern "C" int __declspec(dllexport) __stdcall NP_UnregisterWindowHandle();
extern "C" int __declspec(dllexport) __stdcall NP_RegisterProgramProfileID(short id);
extern "C" int __declspec(dllexport) __stdcall NP_RequestData(short data);
extern "C" int __declspec(dllexport) __stdcall NP_GetData(void *data);
extern "C" int __declspec(dllexport) __stdcall NP_StopCursor();
extern "C" int __declspec(dllexport) __stdcall NP_StartCursor();
extern "C" int __declspec(dllexport) __stdcall NP_StartDataTransmission();
extern "C" int __declspec(dllexport) __stdcall NP_StopDataTransmission();
