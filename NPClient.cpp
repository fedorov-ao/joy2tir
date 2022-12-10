#include "NPClient.hpp"
#include "logging.hpp"
#include "joystick.hpp"

#include <time.h>
#include <cstring> //memset


WinApiJoystick * g_pj = nullptr;

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
          g_pj = new WinApiJoystick(joyID);
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
  pose.pitch = lerp(j.get_axis_value(AxisID::ry), -1.0f, 1.0f, -180.0f, 180.0f);
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
  log_message(
    "x: ", g_pj->get_axis_value(AxisID::x),
    "; y: ", g_pj->get_axis_value(AxisID::y),
    "; z: ", g_pj->get_axis_value(AxisID::z),
    "; rx: ", g_pj->get_axis_value(AxisID::rx),
    "; ry: ", g_pj->get_axis_value(AxisID::ry),
    "; rz: ", g_pj->get_axis_value(AxisID::rz)
  );
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
