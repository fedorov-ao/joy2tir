#include "joystick.hpp"
#include "logging.hpp"
#include "util.hpp"

#include <type_traits>
#include <vector>
#include <map>
#include <functional>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cassert>

#include <hidusage.h>
#include <dinput.h>

int list_legacy_joysticks()
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
        auto desc = joycaps_to_str(jc);
        std::cout << "Joystick " << joyID << " JoyCaps: " << desc << std::endl;
        desc = joyinfoex_to_str(ji);
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

int print_legacy_joystick(int joyID)
{
  auto j = LegacyJoystick(joyID);
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
    char name [256];
    r = GetRawInputDeviceInfoA(ridl.hDevice, RIDI_DEVICENAME, name, &szName);
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
  {
    dwFlags |= RIDEV_REMOVE;
    hwnd = NULL;
  }
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

class RawInputSource
{
public:
  using callback_t = std::function<void(RAWINPUT const &)>;

  std::vector<RawDeviceInfo> get_devices() const
  {
    return get_raw_devices();
  }

  void track(RawDeviceInfo const & rdi, callback_t const & cb)
  {
    auto const handle = rdi.handle;
    auto it = devs_.find(handle);
    if (it == devs_.end())
    {
      devs_[handle] = decltype(devs_)::mapped_type(1, cb);
      //TODO Don't register if same usagePage and usage were already registered
      register_raw_device(rdi, hwnd_, false);
    }
    else
    {
      auto & callbacks = it->second;
#if(0)
      auto end = callbacks.end();
      if (std::find(callbacks.begin(), end, cb) == end)
        callbacks.push_back(cb);
#else
      callbacks.push_back(cb);
#endif
    }
  }

  void run_once()
  {
    MSG msg;
    while (PeekMessageA(&msg, hwnd_, 0, 0, PM_REMOVE) != 0)
    {
      if (msg.message != WM_INPUT)
        continue;
      RAWINPUT raw;
      UINT dwSize = sizeof(raw);
      //TODO Use GetRawInputBuffer() ?
      if (GetRawInputData(reinterpret_cast<HRAWINPUT>(msg.lParam), RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER)) <= 0)
        continue;
      auto const hDevice = raw.header.hDevice;
      auto it = devs_.find(hDevice);
      if (it != devs_.end())
        for (auto & cb : it->second)
          cb(raw);
    }
  }

  RawInputSource(char const * name, bool useMessageWindow=true) : name_(name)
  {
    hwnd_ = create_window(wnd_proc_, name, name, useMessageWindow);
  }

  ~RawInputSource()
  {
    DestroyWindow(hwnd_);
    UnregisterClassA(name_.c_str(), 0);
  }

private:
  static LRESULT __stdcall wnd_proc_(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
  {
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProcA(hWnd, msg, wParam, lParam);
  }

  using devs_t_ = std::map<HANDLE, std::vector<callback_t> >;
  devs_t_ devs_;
  HWND hwnd_ = NULL;
  std::string name_;
};

std::ostream & operator<<(std::ostream & os, RAWINPUTHEADER const & rih)
{
  return os << "dwType: " << rih.dwType << "; dwSize: " << rih.dwSize << "; hDevice: " << rih.hDevice << "; wParam: " << rih.wParam;
}

std::ostream & operator<<(std::ostream & os, RAWINPUT const & ri)
{
  return os << "header: " << ri.header;
}

/* DirectInput8 */
BOOL __stdcall enum_devices_cb(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
  std::cout << dideviceinstancea_to_str(*lpddi) << std::endl;
  return DIENUM_CONTINUE;
}

void test_dinput(int argc, char** argv)
{
  auto const hInstance = GetModuleHandle(NULL);
  auto const dinputVersion = 0x800;
  LPDIRECTINPUT8A pdi = NULL;
  std::cout << "Creating dinput8" << std::endl;
  auto result = DirectInput8Create(hInstance, dinputVersion, IID_IDirectInput8, reinterpret_cast<void**>(&pdi), NULL);
  if (FAILED(result))
    throw std::runtime_error("Failed to create DirectInput8");
  assert(pdi);
  if (argc <= 0)
  {
    std::cout << "modes: enum" << std::endl;
    return;
  }
  auto const mode = argv[0];
  if (strcmp(mode, "enum") == 0)
  {
    std::cout << "Enumerating devices" << std::endl;
    auto devs = get_di8_devices_info(pdi, DI8DEVCLASS_ALL, DIEDFL_ALLDEVICES);
    for (auto const & dev : devs)
      std::cout << dideviceinstancea_to_str(dev.info) << std::endl;
  }
  else if (strcmp(mode, "create") == 0)
  {
    auto const name = argv[1];
    std::cout << "Creating device " << name << std::endl;
    auto devs = get_di8_devices_info(pdi, DI8DEVCLASS_ALL, DIEDFL_ALLDEVICES);
    auto pdid = create_device_by_name(pdi, devs, name);
    std::cout << "Device created" << std::endl;
    DIDEVCAPS caps;
    caps.dwSize = sizeof(caps);
    auto result = pdid->GetCapabilities(&caps);
    if (FAILED(result))
      throw std::runtime_error("Failed to get device caps");
    std::cout << "Caps: " << didevcaps_to_str(caps) << std::endl;
  }
  else
  {
    std::cout << "Unknown mode: " << mode << std::endl;
    return;
  }
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
    return list_legacy_joysticks();
  }
  else if (mode == "print")
  {
    std::stringstream ss (argv[2]);
    int joyID;
    ss >> joyID;
    return print_legacy_joystick(joyID);
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
  else if (mode == "raw_input_source")
  {
    auto const useMessageWindow = strcmp("true", argv[2]) == 0;
    RawInputSource ris ("RawInputSource", useMessageWindow);
    auto deviceInfos = ris.get_devices();
    for (auto & di : deviceInfos)
    {
      std::cout << di << std::endl;
      ris.track(di, [](RAWINPUT const & ri) { std::cout << ri << std::endl; });
    }
    while (true)
    {
      ris.run_once();
    }
    return 0;
  }
  else if (mode == "test_dinput")
  {
    argc -= 2;
    argv += 2;
    test_dinput(argc, argv);
    return 0;
  }
  else
  {
    std::cout << "Unknown mode: " << mode << std::endl;
    return 1;
  }
}
