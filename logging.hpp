#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <string>
#include <fstream>

/* Logging */
std::string get_log_path();
std::fstream& get_log_stream();

template <typename S, typename T>
void log_print(S& s, const T& t)
{
  s << t;
}

template <typename S, typename T, typename... R>
void log_print(S& s, const T& t, const R&... r)
{
  s << t;
  log_print(s, r...);
}

template <typename... T>
void log_message(const T&... t)
{
  auto & stream = get_log_stream();
  log_print(stream, t...);
  stream << std::endl;
}

#endif 
