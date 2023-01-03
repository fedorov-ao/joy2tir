#ifndef LOGGING_HPP
#define LOGGING_HPP

#include "util.hpp"

#include <string>
#include <fstream>
#include <sstream>

/* Logging */
std::string get_log_path();
std::fstream& get_log_stream();

template <typename... T>
void log_message(const T&... t)
{
  auto & stream = get_log_stream();
  strm(stream, t...);
  stream << std::endl;
}

#endif 
