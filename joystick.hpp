#ifndef JOYSTICK_HPP
#define JOYSTICK_HPP

#include <string>
#include <vector>
#include <array>
#include <memory> //shared ptr

#include <windows.h> //legacy joystick API
#include <dinput.h> //DirectInput API

template <typename F, typename T>
T lerp(F const & fv, F const & fb, F const & fe, T const & tb, T const & te)
{
  //tv = a*fv + b
  auto a = (te - tb) / (fe - fb);
  auto b = te - a*fe;
  return a*fv + b;
}

/* Joysticks */
struct LegacyAxisID { enum type { x = 0, first = x, y, z, r, u, v, num }; };

struct AxisID
{
  enum type { x = 0, first = x, y, z, rx, ry, rz, u, v, num };

  static char const * to_cstr(type id);
  static type from_cstr(char const * name);

private:
  static std::array<char const *, AxisID::num> names_;
};

std::pair<UINT, UINT> get_limits_from_joycaps(JOYCAPS const & jc, LegacyAxisID::type id);

DWORD get_pos_from_joyinfoex(JOYINFOEX const & ji, LegacyAxisID::type id);

std::string describe_joycaps(JOYCAPS& jc);

std::string describe_joyinfoex(JOYINFOEX& ji);

class Joystick
{
public:
  virtual float get_axis_value(AxisID::type axisID) const =0;

  virtual ~Joystick() =default;
};

class Updated
{
public:
  virtual void update() =0;

  virtual ~Updated() =default;
};

class LegacyJoystick : public Joystick, public Updated
{
public:
  virtual float get_axis_value(AxisID::type axisID) const override;
  virtual void update() override;

  LegacyJoystick(UINT joyID);

private:
  static LegacyAxisID::type w2n_axis_(AxisID::type ai);

  UINT joyID_;
  std::array<std::pair<UINT, UINT>, LegacyAxisID::num> nativeLimits_;
  std::array<float, AxisID::num> axes_;
};

std::vector<DIDEVICEINSTANCEA> get_devices(LPDIRECTINPUT8A pdi, DWORD devType, DWORD flags);
LPDIRECTINPUTDEVICE8A create_device_by_guid(LPDIRECTINPUT8A pdi, REFGUID instanceGUID);
LPDIRECTINPUTDEVICE8A create_device_by_name(LPDIRECTINPUT8A pdi, std::vector<DIDEVICEINSTANCEA> const & devs, char const * name);

class DInput8Joystick : public Joystick, public Updated
{
public:
  virtual float get_axis_value(AxisID::type axisID) const override;
  virtual void update() override;

  DInput8Joystick(LPDIRECTINPUTDEVICE8A pdid);

private:
  static AxisID::type n2w_axis_(DWORD nai);
  static BOOL __stdcall fill_limits_cb_(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef);

  static DWORD const buffSize_ = 16;
  LPDIRECTINPUTDEVICE8A pdid_;
  std::array<std::pair<LONG, LONG>, AxisID::num> nativeLimits_;
  std::array<float, AxisID::num> axes_;
};

class DInput8JoystickManager : public Updated
{
public:
  std::shared_ptr<DInput8Joystick> make_joystick_by_name(char const * name);
  std::shared_ptr<DInput8Joystick> make_joystick_by_guid(REFGUID instanceGUID);
  virtual void update() override;

  DInput8JoystickManager();

private:
  LPDIRECTINPUT8A pdi_;
  std::vector<std::shared_ptr<DInput8Joystick> > joysticks_;
  std::vector<DIDEVICEINSTANCEA> devs_;
};

class Axis
{
public:
  virtual float get_value() const =0;

  virtual ~Axis() =default;
};

class JoystickAxis : public Axis
{
public:
  virtual float get_value() const;

  JoystickAxis(std::shared_ptr<Joystick> const & spJoystick, AxisID::type axisID);

private:
  std::shared_ptr<Joystick> spJoystick_;
  AxisID::type axisID_;
};

#endif
