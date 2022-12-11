#include "NPClient.hpp"
#include "logging.hpp"
#include "joystick.hpp"

#include <vector>
#include <map>

#include <time.h>
#include <cstring> //memset

/* TrackIR */
void get_signature(char* signature)
{
  //TODO Fill signature
}

void set_trackir_data(tir_data* tir, float yaw, float pitch, float roll, float tx, float ty, float tz)
{
  static unsigned short frame = 0;

  memset(tir, 0, sizeof(*tir));
  //TODO What about other members of tir (checksum)?
  tir->status = 0;
  tir->frame = frame++;
  tir->yaw = -(yaw / 180.0f) * 16384.0f;
  tir->pitch = -(pitch / 180.0f) * 16384.0f;
  tir->roll = -(roll / 180.0f) * 16384.0f;

  tir->tx = -tx * 64.0f;
  tir->ty = ty * 64.0f;
  tir->tz = tz * 64.0f;
}

/* Pose */
enum class PoseMemberID : int { yaw = 0, first = yaw, pitch, roll, x, y, z, num };

struct Pose
{
  float yaw, pitch, roll, x, y, z;
  Pose() =default;
  Pose(float yaw, float pitch, float roll, float x, float y, float z)
    : yaw(yaw), pitch(pitch), roll(roll), x(x), y(y), z(z)
  {}
  ~Pose() =default;
};

std::ostream & operator<<(std::ostream & os, Pose const & pose)
{
  return os << "yaw: " << pose.yaw << "; pitch: " << pose.pitch << "; roll: "<< pose.roll
    << "; x: " << pose.x << "; y: " << pose.y << "; z: " << pose.z;
}

class PoseFactory
{
public:
  virtual Pose make_pose() const =0;

  virtual ~PoseFactory() =default;
};

class AxisPoseFactory : public PoseFactory
{
public:
  virtual Pose make_pose() const;

  void set_axis(PoseMemberID poseMemberID, std::shared_ptr<Axis> const & spAxis, float factor);

  AxisPoseFactory() =default;

private:
  struct AxisData { std::shared_ptr<Axis> spAxis; float factor; } axes_[static_cast<int>(PoseMemberID::num)];
};

Pose AxisPoseFactory::make_pose() const
{
  auto const num = static_cast<int>(PoseMemberID::num);
  float v[num];
  for (int i = static_cast<int>(PoseMemberID::first); i < num; ++i)
  {
    auto const & d = this->axes_[i];
    v[i] = d.spAxis ? d.spAxis->get_value()*d.factor : 0.0f;
  }
  return Pose (
    v[static_cast<int>(PoseMemberID::yaw)],
    v[static_cast<int>(PoseMemberID::pitch)],
    v[static_cast<int>(PoseMemberID::roll)],
    v[static_cast<int>(PoseMemberID::x)],
    v[static_cast<int>(PoseMemberID::y)],
    v[static_cast<int>(PoseMemberID::z)]
  );
}

void AxisPoseFactory::set_axis(PoseMemberID poseMemberID, std::shared_ptr<Axis> const & spAxis, float factor)
{
  auto & d = this->axes_[static_cast<int>(poseMemberID)];
  d.spAxis = spAxis;
  d.factor = factor;
}

Pose make_pose(Joystick const & j)
{
  Pose pose;
  pose.yaw = lerp(j.get_axis_value(AxisID::rx), -1.0f, 1.0f, -180.0f, 180.0f);
  pose.pitch = lerp(j.get_axis_value(AxisID::ry), -1.0f, 1.0f, -180.0f, 180.0f);
  pose.roll = lerp(j.get_axis_value(AxisID::rz), -1.0f, 1.0f, -90.0f, 90.0f);
  pose.x = lerp(j.get_axis_value(AxisID::x), -1.0f, 1.0f, -1.0f, 1.0f);
  pose.y = lerp(j.get_axis_value(AxisID::y), -1.0f, 1.0f, -1.0f, 1.0f);
  pose.z = lerp(j.get_axis_value(AxisID::z), -1.0f, 1.0f, -1.0f, 1.0f);
  return pose;
}

/* Worker functions */
void list_winapi_joysticks()
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

std::vector<std::shared_ptr<Updated> > g_updated;
std::map<UINT, std::shared_ptr<Joystick> > g_joysticks;
std::shared_ptr<PoseFactory> g_poseFactory;

void initialize()
{
  static UINT const posJoyID = 2;
  static UINT const anglesJoyID = 3;
  static struct Mapping
  {
    PoseMemberID poseMemberID;
    UINT joyID;
    AxisID axisID;
    float factor;
  } const mappings[] =
  {
    {PoseMemberID::yaw, anglesJoyID, AxisID::x, 180.0f},
    {PoseMemberID::pitch, anglesJoyID, AxisID::y, 180.0f},
    {PoseMemberID::roll, anglesJoyID, AxisID::z, 90.0f},
    {PoseMemberID::x, posJoyID, AxisID::x, 100.0f},
    {PoseMemberID::y, posJoyID, AxisID::y, 100.0f},
    {PoseMemberID::z, posJoyID, AxisID::z, 100.0f}
  };

  auto spPoseFactory = std::make_shared<AxisPoseFactory>();
  for (auto const & mapping : mappings)
  {
    std::shared_ptr<Joystick> spJoystick;
    auto itJoystick = g_joysticks.find(mapping.joyID);
    if (g_joysticks.end() == itJoystick)
    {
      auto spj = std::make_shared<WinApiJoystick>(mapping.joyID);
      g_joysticks[mapping.joyID] = spj;
      g_updated.push_back(spj);
      spJoystick = spj;
    }
    else
      spJoystick = itJoystick->second;

    auto spAxis = std::make_shared<JoystickAxis>(spJoystick, mapping.axisID);
    spPoseFactory->set_axis(mapping.poseMemberID, spAxis, mapping.factor);
  }
  g_poseFactory = spPoseFactory;
}

void handle(void* data)
{
  for (auto const & sp : g_updated)
    sp->update();

  if (g_poseFactory)
  {
    auto const pose = g_poseFactory->make_pose();
    //auto const pose = Pose(100.0f, 110.0f, 120.0f, 10.0f, 20.0f, 30.0f);
    //log_message("Pose: ", pose);
    set_trackir_data(reinterpret_cast<tir_data*>(data), pose.yaw, pose.pitch, pose.roll, pose.x, pose.y, pose.z);
  }
}

/* Exported Dll functions. */
int __stdcall NP_GetSignature(struct sig_data *signature)
{
  static_assert(sizeof(sig_data) == 400, "sig_data needs to be 400 chars");

  log_message("NP_GetSignature");

  memset(signature, 0, sizeof(sig_data));

  get_signature((char*)signature);

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
