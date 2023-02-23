#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <sstream>

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

#endif
