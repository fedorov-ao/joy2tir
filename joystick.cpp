#include "joystick.hpp"
#include "logging.hpp"

#include <sstream>

/* Joysticks */
char const * AxisID::names_[AxisID::num] = {"x", "y", "z", "rx", "ry", "rz"};

char const * AxisID::to_cstr(AxisID::type id)
{
  return (id < first || id > num) ? "unknown" : names_[id];
}

AxisID::type AxisID::from_cstr(char const * name)
{
  for (int i = 0; i < sizeof(names_)/sizeof(names_[0]); ++i)
  {
    if (strcmp(names_[i], name) == 0)
      return static_cast<type>(i);
  }
  return num;
}

std::pair<UINT, UINT> get_limits_from_joycaps(JOYCAPS const & jc, NativeAxisID::type id)
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

DWORD get_pos_from_joyinfoex(JOYINFOEX const & ji, NativeAxisID::type id)
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

float LegacyJoystick::get_axis_value(AxisID::type axisID) const
{
  return this->axes_[axisID];
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
    if (NativeAxisID::num == nai)
      throw std::logic_error("Bad axis id");
    auto const & l = this->nativeLimits_[nai];
    this->axes_[i] = lerp<DWORD, float>(get_pos_from_joyinfoex(ji, nai), l.first, l.second, -1.0f, 1.0f);
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
  for (int i = NativeAxisID::first; i < NativeAxisID::num; ++i)
  {
    auto const nai = static_cast<NativeAxisID::type>(i);
    this->nativeLimits_[i] = get_limits_from_joycaps(jc, nai);
  }
}

NativeAxisID::type LegacyJoystick::w2n_axis_(AxisID::type ai)
{
  static struct D { AxisID::type ai; NativeAxisID::type nai; } mapping[] = 
  {
    { AxisID::x, NativeAxisID::x },
    { AxisID::y, NativeAxisID::y },
    { AxisID::z, NativeAxisID::z },
    { AxisID::rx, NativeAxisID::r },
    { AxisID::ry, NativeAxisID::u },
    { AxisID::rz, NativeAxisID::v }
  };
  for (auto const & d : mapping)
    if (d.ai == ai)
      return d.nai;
  return NativeAxisID::num;
}

float JoystickAxis::get_value() const
{
  return this->spJoystick_->get_axis_value(this->axisID_);
}

JoystickAxis::JoystickAxis(std::shared_ptr<Joystick> const & spJoystick, AxisID::type axisID)
  : spJoystick_(spJoystick), axisID_(axisID)
{
}
