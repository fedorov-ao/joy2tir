#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <sstream>
#include <dinput.h>

/* String helpers */
template <typename S, typename T>
void strm(S& s, const T& t)
{
  s << t;
}

template <typename S, typename T, typename... R>
void strm(S& s, const T& t, const R&... r)
{
  s << t;
  strm(s, r...);
}

template <typename... T>
std::string stream_to_str(const T&... t)
{
  std::stringstream ss;
  strm(ss, t...);
  return ss.str();
}

/* GUID helpers */
size_t guid2cstr(char * buf, size_t n, REFGUID rguid);

std::string guid2str(REFGUID rguid);

char const * preset_guid2cstr(REFGUID rguid);

GUID cstr2guid(char const * cstr);

GUID str2guid(std::string const & str);

GUID preset_cstr2guid(char const * cstr);

GUID cstr2guidex(char const * cstr);

GUID str2guidex(std::string const & str);

/* Filesystem helpers */
void get_path_to_module_cstr(char* path, size_t szPath);

std::string get_path_to_module();

std::string get_dir_to_module();

std::string & append_to_path(std::string & path, char const * name);

std::string & append_to_path(std::string & path, std::string const & name);

#endif
