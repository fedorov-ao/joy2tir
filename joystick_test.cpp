#include "joystick.hpp"
#include "logging.hpp"

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

#include <hidusage.h>

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

struct RawDeviceInfo
{
  HANDLE handle = 0;
  UINT type = 0;
  std::string name = "";
  UINT usagePage = 0;
  UINT usage = 0;
};

std::ostream & operator<<(std::ostream & os, RawDeviceInfo const & rdi)
{
  return os << "handle: " << rdi.handle << ", type: " << rdi.type << ", name: " << rdi.name << ", usagePage: " << rdi.usagePage << ", usage: " << rdi.usage;
}

std::vector<RawDeviceInfo> get_raw_devices()
{
  UINT uiNumDevices = 0;
  INT r = GetRawInputDeviceList(0, &uiNumDevices, sizeof(RAWINPUTDEVICELIST));
  if (-1 == r)
    throw std::runtime_error("Error getting device number");
  std::vector<RAWINPUTDEVICELIST> rawInputDeviceList (uiNumDevices);
  r = GetRawInputDeviceList(rawInputDeviceList.data(), &uiNumDevices, sizeof(RAWINPUTDEVICELIST));
  if (-1 == r)
    throw std::runtime_error("Error listing devices");
  std::vector<RawDeviceInfo> devices;
  for (UINT i = 0; i < uiNumDevices; ++i)
  {
    auto const & ridl = rawInputDeviceList[i];
    //Get required device name string length
    UINT szName = 0;
    r = GetRawInputDeviceInfoA(ridl.hDevice, RIDI_DEVICENAME, nullptr, &szName);
    if (-1 == r)
      throw std::runtime_error("Error getting device name string length");
    std::string name (szName+1, '\0');
    r = GetRawInputDeviceInfoA(ridl.hDevice, RIDI_DEVICENAME, const_cast<char*>(name.data()), &szName);
    if (-1 == r)
      throw std::runtime_error("Error getting device name");
    RID_DEVICE_INFO ridi;
    UINT szRidi = sizeof(RID_DEVICE_INFO);
    r = GetRawInputDeviceInfoA(ridl.hDevice, RIDI_DEVICEINFO, &ridi, &szRidi);
    if (-1 == r)
      throw std::runtime_error("Error getting device info");
    RawDeviceInfo di;
    di.handle = ridl.hDevice;
    di.type = ridl.dwType;
    di.name = name;
    if (ridl.dwType == RIM_TYPEMOUSE)
    {
      di.usagePage = HID_USAGE_PAGE_GENERIC;
      di.usage = HID_USAGE_GENERIC_MOUSE;
    }
    else if (ridl.dwType == RIM_TYPEKEYBOARD)
    {
      di.usagePage = HID_USAGE_PAGE_GENERIC;
      di.usage = HID_USAGE_GENERIC_KEYBOARD;
    }
    else if (ridl.dwType == RIM_TYPEHID)
    {
      di.usagePage = ridi.hid.usUsagePage;
      di.usage = ridi.hid.usUsage;
    }
    else
    {
      throw std::runtime_error("Unexpected device type");
    }
    devices.push_back(di);
  }
  return devices;
}

void register_raw_device(RawDeviceInfo const & rdi, HWND hwnd, bool remove = false)
{
  static const UINT numRid = 1;
  RAWINPUTDEVICE rid[numRid];
  rid[0].usUsagePage = rdi.usagePage;
  rid[0].usUsage = rdi.usage;
  UINT dwFlags = RIDEV_INPUTSINK;
  if (remove)
    dwFlags |= RIDEV_REMOVE;
  rid[0].dwFlags = dwFlags;
  rid[0].hwndTarget = hwnd;
  if (!RegisterRawInputDevices(rid, numRid, sizeof(RAWINPUTDEVICE)))
    throw std::runtime_error("Failed to register device");
}

HWND create_window(WNDPROC wndProc, char const * className, char const * windowName, bool useMessageWindow)
{
  //Define Window Class
  WNDCLASS wndclass;
  wndclass.style = CS_HREDRAW | CS_VREDRAW;
  wndclass.lpfnWndProc = wndProc;
  wndclass.cbClsExtra = wndclass.cbWndExtra = 0;
  wndclass.hInstance = GetModuleHandleA(nullptr);
  wndclass.hIcon = LoadIconA(nullptr, IDI_APPLICATION);
  wndclass.hCursor = LoadCursorA(nullptr, IDC_ARROW);
  wndclass.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
  wndclass.lpszMenuName = nullptr;
  wndclass.lpszClassName = className;
  //Register Window Class
  if (!RegisterClassA(&wndclass))
    throw std::runtime_error("Failed to register window class");
  //Create Window
  auto hwnd = CreateWindowEx(
    0, className, windowName,
    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    useMessageWindow ? HWND_MESSAGE : nullptr,
    nullptr, wndclass.hInstance, nullptr);
  if (0 == hwnd)
    throw std::runtime_error("Failed to create window");
  if (!useMessageWindow)
  {
    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);
  }
  //TODO Check for error
  return hwnd;
}

LRESULT __stdcall wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_DESTROY)
    PostQuitMessage(0);
  return DefWindowProcA(hWnd, msg, wParam, lParam);
}

int main(int argc, char** argv)
{
  if (argc == 1)
  {
    std::cout
      << "Usage: " << argv[0] << " mode params\n"
      << "mode=list|print|list_raw|window\n"
      << "list: list joysticks\n"
      << "print: print joystick axes values; params: joystick_num\n"
      << "list_raw: list raw devices\n"
      << "window: create and destroy window\n";
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
  else if (mode == "list_raw")
  {
    auto deviceInfos = get_raw_devices();
    for (auto const & d : deviceInfos)
      std::cout << d << std::endl;
    return 0;
  }
  else if (mode == "window")
  {
    char const * className = "Class name";
    auto hwnd = create_window(wnd_proc, className, "Window name", true);
    std::cout << "Window handle: " << hwnd << std::endl;
    auto r = DestroyWindow(hwnd);
    if (0 == r)
      throw std::runtime_error("Error destroying window");
    r = UnregisterClassA(className, 0);
    if (0 == r)
      throw std::runtime_error("Error unregistering window class");
    return 0;
  }
  else
  {
    std::cout << "Unknown mode: " << mode << std::endl;
    return 1;
  }
}
