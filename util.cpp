#include "util.hpp"
#include <windows.h>

size_t guid2cstr(char * buf, size_t n, REFGUID rguid)
{
  char const * fmt = "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX";
  return snprintf(buf, n, fmt,
    rguid.Data1, rguid.Data2, rguid.Data3,
    rguid.Data4[0], rguid.Data4[1], rguid.Data4[2], rguid.Data4[3],
    rguid.Data4[4], rguid.Data4[5], rguid.Data4[6], rguid.Data4[7]);
}

std::string guid2str(REFGUID rguid)
{
  size_t const n = 37;
  char buf[n] = {0};
  guid2cstr(buf, n, rguid);
  return buf;
}

char const * preset_guid2cstr(REFGUID rguid)
{
  static struct { REFGUID rguid; char const * name; } names[] =
  {
    { GUID_SysKeyboard, "SysKeyboard" },
    { GUID_SysMouse, "SysMouse" },
    { GUID_Joystick, "Joystick" }
  };

  for (auto const & p : names)
  {
    if (IsEqualGUID(p.rguid, rguid))
      return p.name;
  }
  return "";
}

GUID cstr2guid(char const * cstr)
{
  char const * fmt = "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX";
  GUID guid;
  sscanf(cstr, fmt,
    &guid.Data1, &guid.Data2, &guid.Data3,
    &guid.Data4[0], &guid.Data4[1], &guid.Data4[2], &guid.Data4[3],
    &guid.Data4[4], &guid.Data4[5], &guid.Data4[6], &guid.Data4[7]);
  return guid;
}

GUID str2guid(std::string const & str)
{
  return cstr2guid(str.data());
}

GUID preset_cstr2guid(char const * cstr)
{
  static struct { GUID guid; char const * name; } names[] =
  {
    { GUID_SysKeyboard, "SysKeyboard" },
    { GUID_SysMouse, "SysMouse" },
    { GUID_Joystick, "Joystick" }
  };

  for (auto const & p : names)
  {
    if (strcmp(p.name, cstr) == 0)
      return p.guid;
  }
  return GUID();
}

GUID cstr2guidex(char const * cstr)
{
  GUID guid = preset_cstr2guid(cstr);
  if (guid == GUID())
    return cstr2guid(cstr);
}

GUID str2guidex(std::string const & str)
{
  return cstr2guidex(str.data());
}

/* https://stackoverflow.com/questions/6924195/get-dll-path-at-runtime */
void get_path_to_module_cstr(char* path, size_t szPath)
{
  HMODULE hm = NULL;

  if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      reinterpret_cast<LPCSTR>(get_path_to_module_cstr), &hm) == 0)
  {
    int ret = GetLastError();
    throw std::runtime_error(stream_to_str("GetModuleHandle failed, error = ", ret));
  }
  if (GetModuleFileName(hm, path, szPath) == 0)
  {
    int ret = GetLastError();
    throw std::runtime_error(stream_to_str("GetModuleFileName failed, error = ", ret));
  }
}

std::string get_path_to_module()
{
  char path[MAX_PATH];
  get_path_to_module_cstr(path, sizeof(path));
  return path;
}

std::string get_dir_to_module()
{
  auto path = get_path_to_module();
  path.erase(path.find_last_of("\\"));
  return path;
}

std::string & append_to_path(std::string & path, char const * name)
{
  if (path.size() && path[-1] != '\\')
    path += '\\';
  path += name;
  return path;
}

std::string & append_to_path(std::string & path, std::string const & name)
{
  append_to_path(path, name.c_str());
  return path;
}
