#include "NPClient.hpp"

#include <string>
#include <fstream>
#include <time.h>
#include <iostream>
#include <sstream>
#include <cstring> //memset

#include <windows.h> //joystick API


/* Logging */
std::string get_log_path()
{
  return "NPClient.log";
}

std::fstream& get_log_stream()
{
  static auto stream = std::fstream(get_log_path(), std::ios::out | std::ios::trunc);
  return stream;
}

template <typename S, typename T>
void log_print(S& s, const T& t)
{
  s << t;
}

template <typename S, typename T, typename... R>
void log_print(S& s, const T& t, const R&... r)
{
  s << t;
  log_print(s, r...);
}

template <typename... T>
void log_message(const T&... t)
{
  auto & stream = get_log_stream();
  log_print(stream, t...);
  stream << std::endl;
}

template <typename T, typename K>
void log_message(const T& t, const K& k)
{
  auto stream = get_log_stream();
  stream << t << k << std::endl;
}


/* TrackIR */
namespace trackir
{
  void get_signature(char* signature)
  {
    //TODO Fill signature
  }
}

void set_trackir_data(void *data, float yaw, float pitch, float roll, float tx, float ty, float tz)
{
  static unsigned short frame;

  tir_data *tir = (tir_data*)data;

  tir->frame = frame++;
  tir->yaw = -(yaw / 180.0f) * 16384.0f;
  tir->pitch = -(pitch / 180.0f) * 16384.0f;
  tir->roll = -(roll / 180.0f) * 16384.0f;

  tir->tx = -tx * 64.0f;
  tir->ty = ty * 64.0f;
  tir->tz = tz * 64.0f;
  //TODO What about other members of tir (checksum, frame)?
}


/* Exported Dll functions. */
int __stdcall NP_GetSignature(struct sig_data *signature)
{
  static_assert(sizeof(sig_data) == 400, "sig_data needs to be 400 chars");

  log_message("NP_GetSignature");

  memset(signature, 0, sizeof(sig_data));

  trackir::get_signature((char*)signature);

  return 0;
}

int __stdcall NP_QueryVersion(short *ver)
{
  log_message("NP_QueryVersion");

  *ver = 0x0400;

  return 0;
}

int __stdcall NP_ReCenter(void)
{
  log_message("NP_ReCenter");

  return 0;
}

int __stdcall NP_RegisterWindowHandle(void *handle)
{
  log_message("NP_RegisterWindowHandle, handle: ", handle);

  return 0;
}

int __stdcall NP_UnregisterWindowHandle(void)
{
  log_message("NP_UnregisterWindowHandle");

  return 0;
}

int __stdcall NP_RegisterProgramProfileID(short id)
{
  log_message("NP_RegisterProgramProfileId, id: ", id);

  return 0;
}

int __stdcall NP_RequestData(short data)
{
  log_message("NP_RequestData: data", data);

  return 0;
}

int __stdcall NP_GetData(void *data)
{
  log_message("NP_GetData");

  memset(data, 0, sizeof(tir_data));

  //TODO Read data from joystick axes
  float yaw=10.0f, pitch=20.0f, roll=30.0f, tx=1.0f, ty=2.0f, tz=3.0f;
  set_trackir_data(data, yaw, pitch, roll, tx, ty, tz);

  return 0;
}

int __stdcall NP_StopCursor(void)
{
  log_message("NP_StopCursor");

  return 0;
}

int __stdcall NP_StartCursor(void)
{
  log_message("NP_StartCursor");

  return 0;
}

int __stdcall NP_StartDataTransmission(void)
{
  log_message("NP_StartDataTransmission");

  return 0;
}

int __stdcall NP_StopDataTransmission(void)
{
  log_message("NP_StopDataTransmission");

  return 0;
}
