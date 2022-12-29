#ifndef JOYSTICK_HPP
#define JOYSTICK_HPP

#include <string>
#include <memory> //shared ptr

#include <windows.h> //joystick API

template <typename F, typename T>
T lerp(F const & fv, F const & fb, F const & fe, T const & tb, T const & te)
{
  //tv = a*fv + b
  auto a = (te - tb) / (fe - fb);
  auto b = te - a*fe;
  return a*fv + b;
}

/* Joysticks */
enum class NativeAxisID { x = 0, first = x, y, z, r, u, v, num };
enum class AxisID {  x = 0, first = x, y, z, rx, ry, rz, num };

char const * axis_id_to_cstr(AxisID id);
AxisID cstr_to_axis_id(char const * name);

std::pair<UINT, UINT> get_limits_from_joycaps(JOYCAPS const & jc, NativeAxisID id);

DWORD get_pos_from_joyinfoex(JOYINFOEX const & ji, NativeAxisID id);

std::string describe_joycaps(JOYCAPS& jc);

std::string describe_joyinfoex(JOYINFOEX& ji);

class Joystick
{
public:
  virtual float get_axis_value(AxisID axisID) const =0;

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
  virtual float get_axis_value(AxisID axisID) const override;
  virtual void update() override;

  LegacyJoystick(UINT joyID);

private:
  static NativeAxisID w2n_axis_(AxisID ai);

  UINT joyID_;
  std::pair<UINT, UINT> nativeLimits_[static_cast<int>(NativeAxisID::num)];
  float axes_[static_cast<int>(AxisID::num)];
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

  JoystickAxis(std::shared_ptr<Joystick> const & spJoystick, AxisID axisID);

private:
  std::shared_ptr<Joystick> spJoystick_;
  AxisID axisID_;
};

#endif
