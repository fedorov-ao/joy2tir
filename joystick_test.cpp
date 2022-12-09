#include "joystick.hpp"
#include "logging.hpp"

#include <sstream>
#include <iostream>
#include <iomanip>

int main(int argc, char** argv)
{
  if (argc == 1)
  {
    std::cout << "Usage: " << argv[0] << " joystick_num" << std::endl;
    return 0;
  }

  std::stringstream ss (argv[1]);
  int joyID;
  ss >> joyID;

  auto j = Joystick(joyID);
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
}
