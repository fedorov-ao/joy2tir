#include "path.hpp"
#include "util.hpp"

#include <windows.h>

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
