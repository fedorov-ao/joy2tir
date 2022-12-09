#include "NPClient.hpp"

#include <string>
#include <fstream>
#include <time.h>
#include <iostream>
#include <sstream>
#include <cstring> //memset

#include <windows.h> //joystick API


template <typename F, typename T>
T lerp(F const & fv, F const & fb, F const & fe, T const & tb, T const & te)
{
  //tv = a*fv + b
  auto a = (te - tb) / (fe - fb);
  auto b = te - a*fe;
  return a*fv + b;
}

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

/* Joysticks */
enum class NativeAxisID { x, first_axis = x, y, z, r, u, v, num_axes };

std::pair<UINT, UINT> get_limits_from_joycaps(JOYCAPS const & jc, NativeAxisID id)
{
  switch (id)
  {
    case NativeAxisID::x: return std::make_pair(jc.wXmin, jc.wXmax);
    case NativeAxisID::y: return std::make_pair(jc.wYmin, jc.wYmax);
    case NativeAxisID::z: return std::make_pair(jc.wZmin, jc.wZmax);
    case NativeAxisID::r: return std::make_pair(jc.wRmin, jc.wRmax);
    case NativeAxisID::u: return std::make_pair(jc.wUmin, jc.wUmax);
    case NativeAxisID::v: return std::make_pair(jc.wVmin, jc.wVmax);
    default: return std::make_pair(0, 0);
  }
}

DWORD get_pos_from_joyinfoex(JOYINFOEX const & ji, NativeAxisID id)
{
  switch (id)
  {
    case NativeAxisID::x: return ji.dwXpos;
    case NativeAxisID::y: return ji.dwYpos;
    case NativeAxisID::z: return ji.dwZpos;
    case NativeAxisID::r: return ji.dwRpos;
    case NativeAxisID::u: return ji.dwUpos;
    case NativeAxisID::v: return ji.dwVpos;
    default: return 0;
  }
}

std::string describe_joycaps(JOYCAPS& jc)
{
  std::stringstream ss;
  ss <<
  "wMid: " << jc.wMid <<
  "; wPid: " << jc.wPid <<
  "; szPname: " << jc.szPname <<
  "; wXmin: " << jc.wXmin <<
  "; wXmax: " << jc.wXmax <<
  "; wYmin: " << jc.wYmin <<
  "; wYmax: " << jc.wYmax <<
  "; wZmin: " << jc.wZmin <<
  "; wZmax: " << jc.wZmax <<
  "; wNumButtons: " << jc.wNumButtons <<
  "; wPeriodMin: " << jc.wPeriodMin <<
  "; wPeriodMax: " << jc.wPeriodMax <<
  "; wRmin: " << jc.wRmin <<
  "; wRmax: " << jc.wRmax <<
  "; wUmin: " << jc.wUmin <<
  "; wUmax: " << jc.wUmax <<
  "; wVmin: " << jc.wVmin <<
  "; wVmax: " << jc.wVmax <<
  "; wCaps: " << jc.wCaps <<
  "; wMaxAxes: " << jc.wMaxAxes <<
  "; wNumAxes: " << jc.wNumAxes <<
  "; wMaxButtons: " << jc.wMaxButtons <<
  "; szRegKey: " << jc.szRegKey <<
  "; szOEMVxD: " << jc.szOEMVxD;
  return ss.str();
}

std::string describe_joyinfoex(JOYINFOEX& ji)
{
  std::stringstream ss;
  ss <<
  "dwSize: " << ji.dwSize <<
  "; dwFlags: " << ji.dwFlags <<
  "; dwXpos: " << ji.dwXpos <<
  "; dwYpos: " << ji.dwYpos <<
  "; dwZpos: " << ji.dwZpos <<
  "; dwRpos: " << ji.dwRpos <<
  "; dwUpos: " << ji.dwUpos <<
  "; dwVpos: " << ji.dwVpos <<
  "; dwButtons: " << ji.dwButtons <<
  "; dwButtonNumber: " << ji.dwButtonNumber <<
  "; dwPOV: " << ji.dwPOV <<
  "; dwReserved1: " << ji.dwReserved1 <<
  "; dwReserved2: " << ji.dwReserved2;
  return ss.str();
}

enum class AxisID {  x, first_axis = x, y, z, rx, ry, rz, num_axes };

class Joystick
{
public:
  float get_axis_value(AxisID axisID) const
  {
    return this->axes_[static_cast<int>(axisID)];
  }

  void update()
  {
    JOYINFOEX ji;
    auto const sji = sizeof(ji);
    memset(&ji, 0, sji);
    ji.dwSize = sji;
    ji.dwFlags = JOY_RETURNALL;
    auto mmr = joyGetPosEx(this->joyID_, &ji);
    if (JOYERR_NOERROR != mmr)
      throw std::runtime_error("Cannot get joystick info");
    for (auto i = static_cast<int>(AxisID::first_axis); i < static_cast<int>(AxisID::num_axes); ++i)
    {
      auto const ai = static_cast<AxisID>(i);
      auto const nai = this->w2n_axis_(ai);
      if (NativeAxisID::num_axes == nai)
        throw std::logic_error("Bad axis id");
      auto const & l = this->nativeLimits_[static_cast<int>(nai)];
      this->axes_[i] = lerp<DWORD, float>(get_pos_from_joyinfoex(ji, nai), l.first, l.second, -1.0f, 1.0f);
    }
  }

  Joystick(UINT joyID) : joyID_(joyID)
  {
    memset(&this->axes_, 0, sizeof(this->axes_));

    JOYCAPS jc;
    auto const sjc = sizeof(jc);
    memset(&jc, 0, sjc);
    auto mmr = joyGetDevCaps(this->joyID_, &jc, sjc);
    if (JOYERR_NOERROR != mmr)
      throw std::runtime_error("Cannot get joystick caps");
    for (auto i = static_cast<int>(NativeAxisID::first_axis); i < static_cast<int>(NativeAxisID::num_axes); ++i)
    {
      auto const nai = static_cast<NativeAxisID>(i);
      this->nativeLimits_[i] = get_limits_from_joycaps(jc, nai);
    }
  }

private:
  static NativeAxisID w2n_axis_(AxisID ai)
  {
    static struct D { AxisID ai; NativeAxisID nai; } mapping[] = 
    {
      { AxisID::x, NativeAxisID::x },
      { AxisID::y, NativeAxisID::y },
      { AxisID::z, NativeAxisID::z },
      { AxisID::rx, NativeAxisID::u },
      { AxisID::ry, NativeAxisID::v },
      { AxisID::rz, NativeAxisID::r }
    };
    for (auto const & d : mapping)
      if (d.ai == ai)
        return d.nai;
    return NativeAxisID::num_axes;
  }

  UINT joyID_;
  std::pair<UINT, UINT> nativeLimits_[static_cast<int>(NativeAxisID::num_axes)];
  float axes_[static_cast<int>(AxisID::num_axes)];
};

Joystick * g_pj = nullptr;

void initialize()
{
  UINT numJoysticks = joyGetNumDevs();
  log_message("Number of joystics: ", numJoysticks);

  JOYINFOEX ji;
  memset(&ji, 0, sizeof(ji));
  ji.dwSize = sizeof(ji);
  ji.dwFlags = JOY_RETURNALL;
  for (UINT joyID = 0; joyID < numJoysticks; ++joyID)
  {
    MMRESULT mmr = joyGetPosEx(joyID, &ji);
    if (JOYERR_NOERROR == mmr)
    {
      log_message("Joystick connected: ", joyID);
      JOYCAPS jc;
      memset(&jc, 0, sizeof(jc));
      mmr = joyGetDevCaps(joyID, &jc, sizeof(jc));
      if (JOYERR_NOERROR == mmr)
      {
        auto desc = describe_joycaps(jc);
        log_message("Joystick ", joyID, " JoyCaps: ", desc);
        desc = describe_joyinfoex(ji);
        log_message("Joystick ", joyID, " JoyInfoEx: ", desc);
        if (joyID == 1)
        {
          log_message("Using joystick ", joyID);
          g_pj = new Joystick(joyID);
        }
      }
      else
      {
        log_message("Error getting joystick ", joyID, " caps; reason: ", mmr);
      }
    }
    else if (JOYERR_UNPLUGGED == mmr)
      log_message("Joystick disconnected: ", joyID);
    else
      log_message("Error accessing joystick: ", joyID, "; reason: ", mmr);
  }
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
  //TODO What about other members of tir (checksum)?
}

struct Pose
{
  float yaw, pitch, roll, x, y, z;
  Pose() =default;
  ~Pose() =default;
};

Pose make_pose(Joystick const & j)
{
  Pose pose;
  pose.yaw = lerp(j.get_axis_value(AxisID::rx), -1.0f, 1.0f, -180.0f, 180.0f);
  pose.pitch = lerp(j.get_axis_value(AxisID::ry), -1.0f, 1.0f, -90.0f, 90.0f);
  pose.roll = lerp(j.get_axis_value(AxisID::rz), -1.0f, 1.0f, -180.0f, 180.0f);
  pose.x = lerp(j.get_axis_value(AxisID::x), -1.0f, 1.0f, -1.0f, 1.0f);
  pose.y = lerp(j.get_axis_value(AxisID::y), -1.0f, 1.0f, -1.0f, 1.0f);
  pose.z = lerp(j.get_axis_value(AxisID::z), -1.0f, 1.0f, -1.0f, 1.0f);
  return pose;
}

std::ostream & operator<<(std::ostream & os, Pose const & pose)
{
  return os << "yaw: " << pose.yaw << "; pitch: " << pose.pitch << "; roll: "<< pose.roll
    << "; x: " << pose.x << "; y: " << pose.y << "; z: " << pose.z;
}

void handle(void* data)
{
  if (nullptr == g_pj)
    return;
  g_pj->update();
  auto const pose = make_pose(*g_pj);
  log_message("Pose: ", pose);
  set_trackir_data(data, pose.yaw, pose.pitch, pose.roll, pose.x, pose.y, pose.z);
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

  initialize();

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

  handle(data);

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
