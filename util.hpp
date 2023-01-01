#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <dinput.h>

size_t guid2cstr(char * buf, size_t n, REFGUID rguid);

std::string guid2str(REFGUID rguid);

char const * preset_guid2cstr(REFGUID rguid);

GUID cstr2guid(char const * cstr);

GUID str2guid(std::string const & str);

GUID preset_cstr2guid(char const * cstr);

GUID cstr2guidex(char const * cstr);

GUID str2guidex(std::string const & str);

#endif
