#include "joystick.hpp"
#include "logging.hpp"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cassert>

/* Joysticks */
decltype(AxisID::names_) AxisID::names_ = {"x", "y", "z", "rx", "ry", "rz", "u", "v"};

char const * AxisID::to_cstr(AxisID::type id)
{
  return (id < first || id > num) ? "unknown" : names_.at(id);
}

AxisID::type AxisID::from_cstr(char const * name)
{
  for (decltype(names_)::size_type i = 0; i < names_.size(); ++i)
  {
    if (strcmp(names_.at(i), name) == 0)
      return static_cast<type>(i);
  }
  return num;
}

std::pair<UINT, UINT> get_limits_from_joycaps(JOYCAPS const & jc, LegacyAxisID::type id)
{
  switch (id)
  {
    case LegacyAxisID::x: return std::make_pair(jc.wXmin, jc.wXmax);
    case LegacyAxisID::y: return std::make_pair(jc.wYmin, jc.wYmax);
    case LegacyAxisID::z: return std::make_pair(jc.wZmin, jc.wZmax);
    case LegacyAxisID::r: return std::make_pair(jc.wRmin, jc.wRmax);
    case LegacyAxisID::u: return std::make_pair(jc.wUmin, jc.wUmax);
    case LegacyAxisID::v: return std::make_pair(jc.wVmin, jc.wVmax);
    default: return std::make_pair(0, 0);
  }
}

DWORD get_pos_from_joyinfoex(JOYINFOEX const & ji, LegacyAxisID::type id)
{
  switch (id)
  {
    case LegacyAxisID::x: return ji.dwXpos;
    case LegacyAxisID::y: return ji.dwYpos;
    case LegacyAxisID::z: return ji.dwZpos;
    case LegacyAxisID::r: return ji.dwRpos;
    case LegacyAxisID::u: return ji.dwUpos;
    case LegacyAxisID::v: return ji.dwVpos;
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

float LegacyJoystick::get_axis_value(AxisID::type axisID) const
{
  return this->axes_.at(axisID);
}

void LegacyJoystick::update()
{
  JOYINFOEX ji;
  auto const sji = sizeof(ji);
  memset(&ji, 0, sji);
  ji.dwSize = sji;
  ji.dwFlags = JOY_RETURNALL;
  auto mmr = joyGetPosEx(this->joyID_, &ji);
  if (JOYERR_NOERROR != mmr)
    throw std::runtime_error("Cannot get joystick info");
  for (int i = AxisID::first; i < AxisID::num; ++i)
  {
    auto const ai = static_cast<AxisID::type>(i);
    auto const nai = this->w2n_axis_(ai);
    if (LegacyAxisID::num == nai)
      throw std::logic_error("Bad axis id");
    auto const & l = this->nativeLimits_.at(nai);
    this->axes_.at(i) = lerp<DWORD, float>(get_pos_from_joyinfoex(ji, nai), l.first, l.second, -1.0f, 1.0f);
  }
}

LegacyJoystick::LegacyJoystick(UINT joyID) : joyID_(joyID)
{
  memset(&this->axes_, 0, sizeof(this->axes_));

  JOYCAPS jc;
  auto const sjc = sizeof(jc);
  memset(&jc, 0, sjc);
  auto mmr = joyGetDevCaps(this->joyID_, &jc, sjc);
  if (JOYERR_NOERROR != mmr)
    throw std::runtime_error("Cannot get joystick caps or joystick is disconnected");
  for (int i = LegacyAxisID::first; i < LegacyAxisID::num; ++i)
  {
    auto const nai = static_cast<LegacyAxisID::type>(i);
    this->nativeLimits_.at(i) = get_limits_from_joycaps(jc, nai);
  }
}

LegacyAxisID::type LegacyJoystick::w2n_axis_(AxisID::type ai)
{
  struct D { AxisID::type ai; LegacyAxisID::type nai; };
  static std::array<D, AxisID::num> mapping = 
  {
    D{ AxisID::x, LegacyAxisID::x },
    D{ AxisID::y, LegacyAxisID::y },
    D{ AxisID::z, LegacyAxisID::z },
    D{ AxisID::rx, LegacyAxisID::r },
    D{ AxisID::ry, LegacyAxisID::u },
    D{ AxisID::rz, LegacyAxisID::v }
  };
  for (auto const & d : mapping)
    if (d.ai == ai)
      return d.nai;
  return LegacyAxisID::num;
}

char const * dierr_to_cstr(HRESULT result)
{
  char const * msg =
    result == DIERR_INPUTLOST ? "DIERR_INPUTLOST" :
    result == DIERR_INVALIDPARAM ? "DIERR_INVALIDPARAM" :
    result == DIERR_NOTACQUIRED ? "DIERR_NOTACQUIRED" :
    result == DIERR_NOTBUFFERED ? "DIERR_NOTBUFFERED" :
    result == DIERR_NOTINITIALIZED ? "DIERR_NOTINITIALIZED" : "UNKNOWN";
  return msg;
}

float DInput8Joystick::get_axis_value(AxisID::type axisID) const
{
  return this->axes_.at(axisID);
}

void DInput8Joystick::update()
{
  std::array<DIDEVICEOBJECTDATA, buffSize_> data;
  DWORD inOut = buffSize_;
  while (true)
  {
    auto const result = pdid_->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), data.data(), &inOut, 0);
    if (FAILED(result))
      throw std::runtime_error(stream_to_str("Failed to get device data: ", dierr_to_cstr(result)));
    if (inOut == 0)
      break;
    //TODO Process only last event for each axis?
    for (decltype(inOut) i = 0; i < inOut; ++i)
    {
      auto const & d = data.at(i);
      auto const ai = this->n2w_axis_(d.dwOfs);
      if (ai == AxisID::num)
        continue;
      auto const & l = this->nativeLimits_.at(ai);
      this->axes_.at(ai) = lerp<DWORD, float>(d.dwData, l.first, l.second, -1.0f, 1.0f);
    }
    inOut = buffSize_;
  }
}

DInput8Joystick::DInput8Joystick(LPDIRECTINPUTDEVICE8A pdid) : pdid_(pdid)
{
  memset(&this->axes_, 0, sizeof(this->axes_));
  if (pdid == NULL)
    throw std::runtime_error("Device pointer is NULL");
  auto result = pdid_->SetDataFormat(&c_dfDIJoystick);
  if (FAILED(result))
    throw std::runtime_error("Failed to set data format");
  DIPROPDWORD dipdBuffSize;
  dipdBuffSize.diph.dwSize = sizeof(DIPROPDWORD);
  dipdBuffSize.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  dipdBuffSize.diph.dwObj = 0;
  dipdBuffSize.diph.dwHow = DIPH_DEVICE;
  dipdBuffSize.dwData = buffSize_;
  result = pdid_->SetProperty(DIPROP_BUFFERSIZE, &dipdBuffSize.diph);
  if (FAILED(result))
    throw std::runtime_error("Failed to set buffer size");
  DIPROPDWORD dipdAxisMode;
  dipdAxisMode.diph.dwSize = sizeof(DIPROPDWORD);
  dipdAxisMode.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  dipdAxisMode.diph.dwObj = 0;
  dipdAxisMode.diph.dwHow = DIPH_DEVICE;
  dipdAxisMode.dwData = DIPROPAXISMODE_ABS;
  result = pdid_->SetProperty(DIPROP_AXISMODE, &dipdAxisMode.diph);
  if (FAILED(result))
    throw std::runtime_error("Failed to set axis mode to absolute");
  result = pdid_->EnumObjects(fill_limits_cb_, this, DIDFT_ABSAXIS);
  if (FAILED(result))
    throw std::runtime_error("Failed to fill limits");
  result = pdid_->Acquire();
  if (FAILED(result))
    throw std::runtime_error("Failed to acquire");
}

AxisID::type DInput8Joystick::n2w_axis_(DWORD nai)
{
  struct D { AxisID::type ai; DWORD nai; };
  static std::array<D, AxisID::num> mapping = 
  {
    D{ AxisID::x, DIJOFS_X },
    D{ AxisID::y, DIJOFS_Y },
    D{ AxisID::z, DIJOFS_Z },
    D{ AxisID::rx, DIJOFS_RX },
    D{ AxisID::ry, DIJOFS_RY },
    D{ AxisID::rz, DIJOFS_RZ },
    D{ AxisID::u, DIJOFS_SLIDER(0) },
    D{ AxisID::v, DIJOFS_SLIDER(1) }
  };
  for (auto const & d : mapping)
    if (d.nai == nai)
      return d.ai;
  return AxisID::num;
}

BOOL __stdcall DInput8Joystick::fill_limits_cb_(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
  auto * that = reinterpret_cast<DInput8Joystick*>(pvRef);
  if ((lpddoi->dwType & DIDFT_ABSAXIS) != 0)
  {
    DIPROPRANGE range;
    range.diph.dwSize = sizeof(DIPROPRANGE);
    range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    range.diph.dwHow = DIPH_BYOFFSET;
    auto const dwOfs = lpddoi->dwOfs;
    range.diph.dwObj = dwOfs;
    auto ai = n2w_axis_(dwOfs);
    if (ai != AxisID::num && that->pdid_->GetProperty(DIPROP_RANGE, &range.diph) == DI_OK)
    {
      auto & nl = that->nativeLimits_.at(ai);
      nl.first = range.lMin;
      nl.second = range.lMax;
    }
  }
  return DIENUM_CONTINUE;
}

BOOL __stdcall fill_devices_cb(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
  using devs_t = std::vector<DIDEVICEINSTANCEA>;
  assert(pvRef);
  auto pDevs = reinterpret_cast<devs_t*>(pvRef);
  pDevs->push_back(*lpddi);
  return DIENUM_CONTINUE;
}

std::vector<DIDEVICEINSTANCEA> get_devices(LPDIRECTINPUT8A pdi, DWORD devType, DWORD flags)
{
  std::vector<DIDEVICEINSTANCEA> devs;
  auto result = pdi->EnumDevices(devType, fill_devices_cb, &devs, flags);
  if (FAILED(result))
    throw std::runtime_error("Failed to enum devices");
  return devs;
}

LPDIRECTINPUTDEVICE8A create_device_by_guid(LPDIRECTINPUT8A pdi, REFGUID instanceGUID)
{
  LPDIRECTINPUTDEVICE8A pdid;
  auto result = pdi->CreateDevice(instanceGUID, &pdid, NULL);
  if (FAILED(result))
    throw std::runtime_error("Failed to create device");
  return pdid;
};

LPDIRECTINPUTDEVICE8A create_device_by_name(LPDIRECTINPUT8A pdi, std::vector<DIDEVICEINSTANCEA> const & devs, char const * name)
{
  auto itDev = std::find_if(devs.begin(), devs.end(),
    [&name](std::remove_reference<decltype(devs)>::type::value_type const & v) { return strcmp(name, v.tszInstanceName) == 0; }
  );
  if (itDev == devs.end())
    throw std::runtime_error("Cannot find device");
  auto const & instanceGuid = itDev->guidInstance;
  return create_device_by_guid(pdi, instanceGuid);
};


std::shared_ptr<DInput8Joystick> DInput8JoystickManager::make_joystick_by_name(char const * name)
{
  auto pdid = create_device_by_name(pdi_, devs_, name);
  auto spJoystick = std::make_shared<DInput8Joystick>(pdid);
  joysticks_.push_back(spJoystick);
  return spJoystick;
}

std::shared_ptr<DInput8Joystick> DInput8JoystickManager::make_joystick_by_guid(REFGUID instanceGUID)
{
  auto pdid = create_device_by_guid(pdi_, instanceGUID);
  auto spJoystick = std::make_shared<DInput8Joystick>(pdid);
  joysticks_.push_back(spJoystick);
  return spJoystick;
}

std::vector<DIDEVICEINSTANCEA> const & DInput8JoystickManager::get_joysticks_list() const
{
  return devs_;
}

void DInput8JoystickManager::update()
{
  for (auto & j : joysticks_)
    j->update();
}

DInput8JoystickManager::DInput8JoystickManager() : pdi_(NULL), joysticks_(), devs_()
{
  auto const hInstance = GetModuleHandle(NULL);
  auto const dinputVersion = 0x800;
  auto result = DirectInput8Create(hInstance, dinputVersion, IID_IDirectInput8, reinterpret_cast<void**>(&pdi_), NULL);
  if (FAILED(result))
    throw std::runtime_error("Failed to create DirectInput8");
  assert(pdi_);
  devs_ = get_devices(pdi_, DI8DEVTYPE_JOYSTICK, DIEDFL_ALLDEVICES);
}

float JoystickAxis::get_value() const
{
  return this->spJoystick_->get_axis_value(this->axisID_);
}

JoystickAxis::JoystickAxis(std::shared_ptr<Joystick> const & spJoystick, AxisID::type axisID)
  : spJoystick_(spJoystick), axisID_(axisID)
{
}
