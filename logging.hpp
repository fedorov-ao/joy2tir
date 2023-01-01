#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <string>
#include <fstream>
#include <sstream>

/* Helpers */
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
