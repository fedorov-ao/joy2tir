#ifndef PATH_HPP
#define PATH_HPP

#include <string>

/* Filesystem helpers */
void get_path_to_module_cstr(char* path, size_t szPath);

std::string get_path_to_module();

std::string get_dir_to_module();

std::string & append_to_path(std::string & path, char const * name);

std::string & append_to_path(std::string & path, std::string const & name);

#endif
