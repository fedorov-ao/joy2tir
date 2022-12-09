#ifndef JOYSTICK_HPP
#define JOYSTICK_HPP

#include <string>

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
enum class NativeAxisID { x = 0, first_axis = x, y = 1, z = 2, r = 3, u = 4, v = 5, num_axes = 6 };
enum class AxisID {  x = 0, first_axis = x, y = 1, z = 2, rx = 3, ry = 4, rz = 5, num_axes = 6 };

std::pair<UINT, UINT> get_limits_from_joycaps(JOYCAPS const & jc, NativeAxisID id);

DWORD get_pos_from_joyinfoex(JOYINFOEX const & ji, NativeAxisID id);

std::string describe_joycaps(JOYCAPS& jc);

std::string describe_joyinfoex(JOYINFOEX& ji);

class Joystick
{
public:
  float get_axis_value(AxisID axisID) const;
  void update();

  Joystick(UINT joyID);

private:
  static NativeAxisID w2n_axis_(AxisID ai);

  UINT joyID_;
  std::pair<UINT, UINT> nativeLimits_[static_cast<int>(NativeAxisID::num_axes)];
  float axes_[static_cast<int>(AxisID::num_axes)];
};

#endif
