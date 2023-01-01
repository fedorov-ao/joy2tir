#include "NPClient.hpp"
#include "logging.hpp"
#include "joystick.hpp"
#include "util.hpp"
#include "nlohmann/json.hpp"

#include <vector>
#include <map>
#include <sstream>

#include <time.h>
#include <cstring> //memset

template <class R, class C, class K>
R get_d(C const & config, K&& key, R&& dfault)
try {
  return config.at(key).template get<R>();
} catch (nlohmann::json::out_of_range const & e)
{
  return dfault;
}

/* TrackIR */
/*
tir2joy-master/NPClient.h:55
// DATA FIELDS
#define NPControl   8 // indicates a control data field
            // the second parameter of a message bearing control data information contains a packed data format.
            // The High byte indicates what the data is, and the Low byte contains the actual data
// roll, pitch, yaw
#define NPRoll    1 // +/- 16383 (representing +/- 180) [data = input - 16383]
#define NPPitch   2 // +/- 16383 (representing +/- 180) [data = input - 16383]
#define NPYaw   4 // +/- 16383 (representing +/- 180) [data = input - 16383]

// x, y, z - remaining 6dof coordinates
#define NPX     16  // +/- 16383 [data = input - 16383]
#define NPY     32  // +/- 16383 [data = input - 16383]
#define NPZ     64  // +/- 16383 [data = input - 16383]

// raw object position from imager
#define NPRawX    128 // 0..25600 (actual value is multiplied x 100 to pass two decimal places of precision)  [data = input / 100]
#define NPRawY    256  // 0..25600 (actual value is multiplied x 100 to pass two decimal places of precision)  [data = input / 100]
#define NPRawZ    512  // 0..25600 (actual value is multiplied x 100 to pass two decimal places of precision)  [data = input / 100]

// x, y, z deltas from raw imager position
#define NPDeltaX    1024 // +/- 2560 (actual value is multiplied x 10 to pass two decimal places of precision)  [data = (input / 10) - 256]
#define NPDeltaY    2048 // +/- 2560 (actual value is multiplied x 10 to pass two decimal places of precision)  [data = (input / 10) - 256]
#define NPDeltaZ    4096 // +/- 2560 (actual value is multiplied x 10 to pass two decimal places of precision)  [data = (input / 10) - 256]

// raw object position from imager
#define NPSmoothX   8192    // 0..32766 (actual value is multiplied x 10 to pass one decimal place of precision) [data = input / 10]
#define NPSmoothY   16384  // 0..32766 (actual value is multiplied x 10 to pass one decimal place of precision) [data = input / 10]
#define NPSmoothZ   32768  // 0..32766 (actual value is multiplied x 10 to pass one decimal place of precision) [data = input / 10]


//////////////////
/// Typedefs /////////////////////////////////////////////////////////////////////
/////////////////

// NPESULT values are returned from the Game Client API functions.
//
typedef enum tagNPResult
{
  NP_OK = 0,
  NP_ERR_DEVICE_NOT_PRESENT,
  NP_ERR_UNSUPPORTED_OS,
  NP_ERR_INVALID_ARG,
  NP_ERR_DLL_NOT_FOUND,
  NP_ERR_NO_DATA,
  NP_ERR_INTERNAL_DATA

} NPRESULT;

typedef struct tagTrackIRSignature
{
  char DllSignature[200];
  char AppSignature[200];

} SIGNATUREDATA, *LPTRACKIRSIGNATURE;

typedef struct tagTrackIRData
{
  unsigned short wNPStatus;
  unsigned short wPFrameSignature;
  unsigned long  dwNPIOData;

  float fNPRoll;
  float fNPPitch;
  float fNPYaw;
  float fNPX;
  float fNPY;
  float fNPZ;
  float fNPRawX;
  float fNPRawY;
  float fNPRawZ;
  float fNPDeltaX;
  float fNPDeltaY;
  float fNPDeltaZ;
  float fNPSmoothX;
  float fNPSmoothY;
  float fNPSmoothZ;

} TRACKIRDATA, *LPTRACKIRDATA;

linuxtrack-wine-0.1/src/ltrnp.c:155
int __stdcall NPCLIENT_NP_GetData(void *data)
{
  static short frame;
  unsigned int counter;
  struct tir_data *tir = data;

  memset(tir, 0, sizeof *tir);

  if(ltr_get_camera_update(&tir->yaw, &tir->pitch, &tir->roll,
  &tir->tx, &tir->ty, &tir->tz, &counter) < 0) {
  logmsg("ltr_get_camera_update failed!\n");
  }
  tir->frame = frame++;

  tir->yaw = -(tir->yaw / 180.0f) * 16384.0f;
  tir->pitch = -(tir->pitch / 180.0f) * 16384.0f;
  tir->roll = -(tir->roll / 180.0f) * 16384.0f;

  tir->tx = -tir->tx * 64.0f;
  tir->ty = tir->ty * 64.0f;
  tir->tz = tir->tz * 64.0f;

  return 0;
}
*/

/* Pose */
struct PoseMemberID
{
  enum type { yaw = 0, first = yaw, pitch, roll, x, y, z, num };

  static type from_cstr(char const * name)
  {
    for (int i = 0; i < names_.size(); ++i)
    {
      if (strcmp(names_.at(i), name) == 0)
        return static_cast<type>(i);
    }
    return num;
  }

  static char const * to_cstr(type id)
  {
    return (id < first || id > num) ? "unknown" : names_.at(id);
  }

private:
  static std::array<char const *, num> names_;
};

decltype(PoseMemberID::names_) PoseMemberID::names_ = {"yaw", "pitch", "roll", "x", "y", "z"};

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
  using limits_t = std::pair<float, float>;

  virtual Pose make_pose() const;

  void set_mapping(PoseMemberID::type poseMemberID, std::shared_ptr<Axis> const & spAxis, limits_t const & limits);
  void set_axis(PoseMemberID::type poseMemberID, std::shared_ptr<Axis> const & spAxis);
  void set_limits(PoseMemberID::type poseMemberID, limits_t const & limits);

  AxisPoseFactory();

private:
  struct AxisData { std::shared_ptr<Axis> spAxis; limits_t limits; };
  std::array<AxisData, PoseMemberID::num> axes_;
};

Pose AxisPoseFactory::make_pose() const
{
  auto const num = PoseMemberID::num;
  std::array<float, num> v;
  for (size_t i = PoseMemberID::first; i < num; ++i)
  {
    auto const & d = this->axes_.at(i);
    v.at(i) = d.spAxis ? lerp(d.spAxis->get_value(), -1.0f, 1.0f, d.limits.first, d.limits.second) : 0.0f;
  }
  return Pose (
    v.at(PoseMemberID::yaw),
    v.at(PoseMemberID::pitch),
    v.at(PoseMemberID::roll),
    v.at(PoseMemberID::x),
    v.at(PoseMemberID::y),
    v.at(PoseMemberID::z)
  );
}

void AxisPoseFactory::set_mapping(PoseMemberID::type poseMemberID, std::shared_ptr<Axis> const & spAxis, AxisPoseFactory::limits_t const & limits)
{
  auto & d = this->axes_.at(poseMemberID);
  d.spAxis = spAxis;
  d.limits = limits;
}

void AxisPoseFactory::set_axis(PoseMemberID::type poseMemberID, std::shared_ptr<Axis> const & spAxis)
{
  auto & d = this->axes_.at(poseMemberID);
  d.spAxis = spAxis;
}

void AxisPoseFactory::set_limits(PoseMemberID::type poseMemberID, AxisPoseFactory::limits_t const & limits)
{
  auto & d = this->axes_.at(poseMemberID);
  d.limits = limits;
}

AxisPoseFactory::AxisPoseFactory()
{
  for (auto & d : this->axes_)
  {
    d.spAxis = nullptr;
    d.limits = limits_t(-1.0f, 1.0f);
  }
}

/* tir_data setter */
struct TIRData
{
public:
  enum type {
    NPControl = 8,
    NPRoll = 1, NPPitch = 2, NPYaw = 4,
    NPX = 16, NPY = 32, NPZ = 64,
    NPRawX = 128, NPRawY = 256, NPRawZ = 512,
    NPDeltaX = 1024, NPDeltaY = 2048, NPDeltaZ = 4096,
    NPSmoothX = 8192, NPSmoothY = 16384, NPSmoothZ = 32768
  };

  static short from_cstr(char const * name)
  {
    for (auto const & d : cstr2value_)
      if (0 == strcmp(d.name, name))
        return d.value;
    return 0;
  }

  static char const * to_cstr(short value)
  {
    for (auto const & d : cstr2value_)
      if (d.value == value)
        return d.name;
    return "";
  }

  template <class C>
  static void to_cstr_cb(int value, C && cb)
  {
    for (auto const & d : cstr2value_)
      if (d.value & value)
        cb(d.name);
  }

  static std::string to_str(short value)
  {
    std::stringstream ss;
    bool first = true;
    TIRData::to_cstr_cb(
      value,
      [&ss, &first](char const * name)
      {
        if (!first)
          ss << ", ";
        else
          first = false;
        ss << "\"" << name << "\"";
      }
    );
    return ss.str();
  }

private:
  struct D { char const * name; int value; };
  static std::array<D, 16> cstr2value_;
};

decltype(TIRData::cstr2value_) TIRData::cstr2value_ = {
  D{ "control", TIRData::NPControl },
  D{ "roll", TIRData::NPRoll },
  D{ "pitch", TIRData::NPPitch },
  D{ "yaw", TIRData::NPYaw },
  D{ "x", TIRData::NPX },
  D{ "y", TIRData::NPY },
  D{ "z", TIRData::NPZ },
  D{ "rawx", TIRData::NPRawX },
  D{ "rawy", TIRData::NPRawY },
  D{ "rawz", TIRData::NPRawZ },
  D{ "deltax", TIRData::NPDeltaX },
  D{ "deltay", TIRData::NPDeltaY },
  D{ "deltaz", TIRData::NPDeltaZ },
  D{ "smoothx", TIRData::NPSmoothX },
  D{ "smoothy", TIRData::NPSmoothY },
  D{ "smoothz", TIRData::NPSmoothZ }
};

class TIRDataSetter
{
public:
  /* pose yaw, pitch, roll are +/- 180.0f degrees; pose x, y, z, are +/- 256.0f centimeters */
  void set_trackir_data(tir_data* tir, Pose const & pose)
  {
    if (erase_)
      memset(tir, 0, sizeof(*tir));
    //TODO What about other members of tir (checksum)?
    tir->status = 0;
    tir->frame = frame_++;
    if (data_ & TIRData::NPYaw) tir->yaw = convert_angle_(-pose.yaw);
    if (data_ & TIRData::NPPitch) tir->pitch = convert_angle_(-pose.pitch);
    if (data_ & TIRData::NPRoll) tir->roll = convert_angle_(-pose.roll);

    if (data_ & TIRData::NPX) tir->tx = convert_t_(-pose.x);
    if (data_ & TIRData::NPY) tir->ty = convert_t_(pose.y);
    if (data_ & TIRData::NPZ) tir->tz = convert_t_(pose.z);

    if (data_ & TIRData::NPRawX) tir->rawx = convert_raw_(-pose.x);
    if (data_ & TIRData::NPRawY) tir->rawy = convert_raw_(pose.y);
    if (data_ & TIRData::NPRawZ) tir->rawz = convert_raw_(pose.z);

    if (data_ & TIRData::NPDeltaX) tir->deltax = convert_delta_(-pose.x, rawx_);
    if (data_ & TIRData::NPDeltaY) tir->deltay = convert_delta_(pose.y, rawy_);
    if (data_ & TIRData::NPDeltaZ) tir->deltaz = convert_delta_(pose.z, rawz_);

    if (data_ & TIRData::NPSmoothX) tir->smoothx = convert_smooth_(-pose.x);
    if (data_ & TIRData::NPSmoothY) tir->smoothy = convert_smooth_(pose.y);
    if (data_ & TIRData::NPSmoothZ) tir->smoothz = convert_smooth_(pose.z);
  }

  void set_data(short data) { data_ = data; }
  short get_data() const { return data_; }

  void set_erase(bool erase) { erase_ = erase; }
  bool get_erase() const { return erase_; }

  void set_frame(unsigned short frame) { frame_ = frame; }
  unsigned short get_frame() const { return frame_; }

  TIRDataSetter() {}

private:
  float convert_angle_(float pa) const { return pa / 180.0f * 16384.0f; }
  float convert_t_(float pc) const { return pc * 64.0f; }
  float convert_raw_(float pc) const { return (pc + 256.0f) * 50.0f; }
  float convert_delta_(float pc, float & rawOld)
  {
    pc = convert_raw_(pc);
    if (pc == rawOld)
      return 0.0f;
    auto const r = pc - rawOld;
    rawOld = pc;
    return r;
  }
  float convert_smooth_(float pc) const { return (pc + 256.0f) * 64.0f; }

  bool erase_ = true;
  short data_ = 0;
  unsigned short frame_ = 0;
  float rawx_ = 0.0f, rawy_ = 0.0f, rawz_ = 0.0f;
};

/* Worker functions */
void list_legacy_joysticks()
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

struct LegacyJoystickInfo
{
  JOYCAPS joyCaps;
  JOYINFOEX joyInfo;
};

std::vector<LegacyJoystickInfo> get_legacy_joysticks_info()
{
  std::vector<LegacyJoystickInfo> r;
  auto numJoysticks = joyGetNumDevs();
  for (decltype(numJoysticks) joyID = 0; joyID < numJoysticks; ++joyID)
  {
    LegacyJoystickInfo info;
    info.joyInfo.dwSize = sizeof(info.joyInfo);
    info.joyInfo.dwFlags = JOY_RETURNALL;
    auto mmr = joyGetPosEx(joyID, &info.joyInfo);
    if (JOYERR_NOERROR != mmr)
      continue;
    mmr = joyGetDevCaps(joyID, &info.joyCaps, sizeof(info.joyCaps));
    if (JOYERR_NOERROR != mmr)
      continue;
    r.push_back(info);
  }
  return r;
}

/* Main class */
class Main
{
public:
  void set_tir_data_fields(short dataFields);
  void fill_tir_data(void * data);
  void update();

  Main(char const * configName);

private:
  std::vector<std::shared_ptr<Updated> > updated_;
  std::map<std::string, std::shared_ptr<Joystick> > joysticks_;
  std::shared_ptr<DInput8JoystickManager> spDI8JoyManager_;
  std::shared_ptr<PoseFactory> spPoseFactory_;
  TIRDataSetter tirDataSetter_;
};

std::shared_ptr<Main> g_spMain;

Main::Main(char const * configName)
{
  spDI8JoyManager_ = std::make_shared<DInput8JoystickManager>();
  updated_.push_back(spDI8JoyManager_);

  std::ifstream configStream (configName);
  auto config = nlohmann::json::parse(configStream);

  auto const printJoysticks = get_d<bool>(config, "printJoysticks", false);
  if (printJoysticks)
  {
    log_message("Legacy joysticks");
    auto const legacyJoysticksInfo = get_legacy_joysticks_info();
    for (decltype(legacyJoysticksInfo)::size_type joyID = 0; joyID < legacyJoysticksInfo.size(); ++joyID)
    {
      auto const & info = legacyJoysticksInfo.at(joyID);
      log_message("joystick ", joyID, ": info: ", describe_joyinfoex(info.joyInfo), "; caps: ", describe_joycaps(info.joyCaps));
    }
    log_message("DirectInput8 joysticks");
    for (auto const & d : spDI8JoyManager_->get_joysticks_info())
      log_message(dideviceinstancea_to_str(d));
  }

  auto const tirDataFieldsName = "tirDataFields";
  short tirDataFields = -1;
  if (config.contains(tirDataFieldsName))
  {
    nlohmann::json const tirDataFieldNames = config.at(tirDataFieldsName);
    tirDataFields = 0;
    for (auto const & n : tirDataFieldNames)
      tirDataFields |= TIRData::from_cstr(n.get<std::string>().c_str());
    log_message("TIR data fields to be filled: ", tirDataFieldNames, " (", tirDataFields, ")");
  }
  tirDataSetter_.set_data(tirDataFields);

  tirDataSetter_.set_erase(get_d(config, "tirEraseData", true));
  tirDataSetter_.set_frame(get_d(config, "tirStartFrame", 0));

  auto const & joysticks = config.at("joysticks");
  for (auto const & j : joysticks.items())
  {
    auto const name = j.key();
    auto const cfg = j.value();
    try {
      auto const type = get_d<std::string>(cfg, "type", "");
      if (type == "legacy")
      {
        auto const joyID = get_d<UINT>(cfg, "id", 0);
        auto const spj = std::make_shared<LegacyJoystick>(joyID);
        joysticks_[name] = spj;
        updated_.push_back(spj);
      }
      else if (type == "di8")
      {
        assert(spDI8JoyManager_);
        std::shared_ptr<Joystick> spj;
        auto const joyNameStr = get_d<std::string>(cfg, "name", "");
        if (joyNameStr.size())
          spj = spDI8JoyManager_->make_joystick_by_name(joyNameStr.c_str());
        else
        {
          auto const joyGuidStr = get_d<std::string>(cfg, "guid", "");
          if (joyGuidStr.size())
          {
            spj = spDI8JoyManager_->make_joystick_by_guid(str2guid(joyGuidStr.c_str()));
          }
          else
            throw std::runtime_error("Need to specify either name or guid");
        }
        joysticks_[name] = spj;
      }
      else
        throw std::runtime_error(stream_to_str("Unknown joystick type: '", type, "'"));
    } catch (std::runtime_error & e)
    {
      log_message("Could not create joystick '", name, "' (", e.what(), ")");
    }
  }

  auto spPoseFactory = std::make_shared<AxisPoseFactory>();
  auto & mappings = config.at("mapping");
  for (auto & m : mappings)
  {
    try {
      auto const tirAxisName = m.at("tirAxis").get<std::string>();
      auto poseMemberID = PoseMemberID::from_cstr(tirAxisName.c_str());
      auto const joyName = m.at("joystick").get<std::string>();
      auto axisID = AxisID::from_cstr(m.at("joyAxis").get<std::string>().c_str());
      auto limits = AxisPoseFactory::limits_t(-1.0f, 1.0f);
      if (m.contains("limits"))
      {
        auto const & l = m.at("limits");
        limits.first = l[0].get<float>();
        limits.second = l[1].get<float>();
      }

      auto itJoystick = joysticks_.find(joyName);
      if (joysticks_.end() == itJoystick)
      {
        log_message("Could not create mapping for TIR axis '", tirAxisName, "' (joystick '", joyName, "' was not created)");
        continue;
      }

      auto spAxis = std::make_shared<JoystickAxis>(itJoystick->second, axisID);
      spPoseFactory->set_mapping(poseMemberID, spAxis, limits);
    }
    catch (std::exception & e)
    {
      log_message("Could not create mapping ", m, " (", e.what(), ")");
    }
  }
  spPoseFactory_ = spPoseFactory;
}

void Main::set_tir_data_fields(short dataFields)
{
  auto const dataFieldsStr = TIRData::to_str(dataFields);
  log_message("Application requests TIR data fields to be filled: [", dataFieldsStr, "] (", dataFields, ")");

  auto const configDataFields = tirDataSetter_.get_data();
  if (configDataFields == -1)
  {
    tirDataSetter_.set_data(dataFields);
    log_message("Will fill TIR data fields: [", dataFieldsStr, "] (", dataFields, ")");
  }
  else
  {
    log_message("Will fill TIR data fields: [", TIRData::to_str(configDataFields), "] (", configDataFields, "), as specified in config");
  }
}

void Main::update()
{
  for (auto const & sp : updated_)
    try
    {
      sp->update();
    }
    catch (std::runtime_error & e)
    {
      log_message(e.what());
    }
}

void Main::fill_tir_data(void * data)
{
  auto const pose = spPoseFactory_->make_pose();
  //auto const pose = Pose(100.0f, 110.0f, 120.0f, 10.0f, 20.0f, 30.0f);
  //log_message("Pose: ", pose);
  tirDataSetter_.set_trackir_data(reinterpret_cast<tir_data*>(data), pose);
}

/* Exported Dll functions. */
int __stdcall NP_GetSignature(struct sig_data *signature)
{
  static_assert(sizeof(sig_data) == 400, "sig_data needs to be 400 chars");

  log_message("NP_GetSignature");

  static auto const szd = sizeof(sig_data);
  memset(signature, 0, szd);
  memcpy(signature, &sigdata, szd);

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

  g_spMain = std::make_shared<Main>("NPClient.json");

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

int __stdcall NP_RequestData(short dataFields)
{
  log_message("NP_RequestData");

  if (g_spMain)
    g_spMain->set_tir_data_fields(dataFields);

  return 0;
}

int __stdcall NP_GetData(void *data)
{
  //log_message("NP_GetData");

  if (g_spMain)
  {
    try {
      g_spMain->update();
      g_spMain->fill_tir_data(data);
    } catch (std::exception & e)
    {
      log_message("Exception in main loop: ", e.what());
    }
  }

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
