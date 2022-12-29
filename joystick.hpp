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
struct NativeAxisID { enum type { x = 0, first = x, y, z, r, u, v, num }; };

struct AxisID
{
  enum type { x = 0, first = x, y, z, rx, ry, rz, num };

  static char const * to_cstr(type id);
  static type from_cstr(char const * name);

private:
  static char const * names_[num];
};


std::pair<UINT, UINT> get_limits_from_joycaps(JOYCAPS const & jc, NativeAxisID::type id);

DWORD get_pos_from_joyinfoex(JOYINFOEX const & ji, NativeAxisID::type id);

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
  static NativeAxisID::type w2n_axis_(AxisID::type ai);

  UINT joyID_;
  std::pair<UINT, UINT> nativeLimits_[NativeAxisID::num];
  float axes_[AxisID::num];
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
