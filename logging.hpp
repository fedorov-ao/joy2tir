#ifndef LOGGING_HPP
#define LOGGING_HPP

#include "util.hpp"

#include <string>
#include <fstream>

/* Logging */
namespace logging
{

enum class LogLevel : int { notset=0, trace=1, debug=2, info=3, error=4 };

char const * ll2n(LogLevel level);
LogLevel n2ll(char const * name);
LogLevel n2ll(std::string const & name);

void set_log_level(LogLevel level);
LogLevel get_log_level();

std::string get_log_path();
std::fstream& get_log_stream();

template <typename... T>
void log(char const * source, LogLevel level, const T&... t)
{
  if (static_cast<int>(get_log_level()) > static_cast<int>(level))
    return;
  auto & stream = get_log_stream();
  strm(stream, source, ll2n(level), t...);
  stream << std::endl;
}

} //logging

#endif 
