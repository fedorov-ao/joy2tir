#include "guid.hpp"

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
