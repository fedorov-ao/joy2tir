#include "joystick.hpp"
#include "logging.hpp"

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

int list_winapi_joysticks()
{
  UINT numJoysticks = joyGetNumDevs();
  std::cout << "Number of joystics: " << numJoysticks << std::endl;

  JOYINFOEX ji;
  memset(&ji, 0, sizeof(ji));
  ji.dwSize = sizeof(ji);
  ji.dwFlags = JOY_RETURNALL;
  for (UINT joyID = 0; joyID < numJoysticks; ++joyID)
  {
    MMRESULT mmr = joyGetPosEx(joyID, &ji);
    if (JOYERR_NOERROR == mmr)
    {
      std::cout << "Joystick connected: " << joyID << std::endl;
      JOYCAPS jc;
      memset(&jc, 0, sizeof(jc));
      mmr = joyGetDevCaps(joyID, &jc, sizeof(jc));
      if (JOYERR_NOERROR == mmr)
      {
        auto desc = describe_joycaps(jc);
        std::cout << "Joystick " << joyID << " JoyCaps: " << desc << std::endl;
        desc = describe_joyinfoex(ji);
        std::cout << "Joystick " << joyID << " JoyInfoEx: " << desc << std::endl;
      }
      else
      {
        std::cout << "Error getting joystick " << joyID << " caps; reason: " << mmr << std::endl;
      }
    }
    else if (JOYERR_UNPLUGGED == mmr)
      std::cout << "Joystick disconnected: " << joyID << std::endl;
    else
      std::cout << "Error accessing joystick: " << joyID << "; reason: " << mmr << std::endl;
  }
  return 0;
}

int print_winapi_joystick(int joyID)
{
  auto j = WinApiJoystick(joyID);
  std::cout << std::fixed << std::setprecision(2) << std::showpos;
  while(true)
  {
    j.update();
    std::cout <<
      "x: " << j.get_axis_value(AxisID::x) <<
      "; y: " << j.get_axis_value(AxisID::y) <<
      "; z: " << j.get_axis_value(AxisID::z) <<
      "; rx: " << j.get_axis_value(AxisID::rx) <<
      "; ry: " << j.get_axis_value(AxisID::ry) <<
      "; rz: " << j.get_axis_value(AxisID::rz) <<
      std::endl;
  }
  return 0;
}

int main(int argc, char** argv)
{
  if (argc == 1)
  {
    std::cout
      << "Usage: " << argv[0] << " mode params\n"
      << "mode=list|print\n"
      << "list: list joysticks\n"
      << "print: print joystick axes values; params: joystick_num"
      << std::endl;
    return 0;
  }

  std::string mode (argv[1]);
  if (mode == "list")
  {
    return list_winapi_joysticks();
  }
  else if (mode == "print")
  {
    std::stringstream ss (argv[2]);
    int joyID;
    ss >> joyID;
    return print_winapi_joystick(joyID);
  }
  else
  {
    std::cout << "Unknown mode: " << mode << std::endl;
    return 1;
  }
}
