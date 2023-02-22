#include "logging.hpp"
#include <cstdlib>
#include <cstring>

namespace logging
{

static struct { char const * name; LogLevel level; } g_logLevelNames[] = {
  { "NOTSET", LogLevel::notset },
  { "TRACE", LogLevel::trace },
  { "DEBUG", LogLevel::debug },
  { "INFO", LogLevel::info },
  { "ERROR", LogLevel::error }
};

char const * ll2n(LogLevel level)
{
  for (auto const & p : g_logLevelNames)
    if (p.level == level) 
      return p.name;
  return "";
}

LogLevel n2ll(char const * name)
{
  for (auto const & p : g_logLevelNames)
    if (std::strcmp(p.name, name) == 0) 
      return p.level;
  return LogLevel::notset;
}

LogLevel n2ll(std::string const & name)
{
  return n2ll(name.c_str());
}

static LogLevel g_logLevel;

void set_log_level(LogLevel level)
{
  g_logLevel = level;
}

LogLevel get_log_level()
{
  return g_logLevel;
}

std::string get_log_path()
{
  std::string logPath;
  if (auto envLogPath = std::getenv("JOY2TIR_LOG"))
  {
    logPath = envLogPath;
  }
  else
  {
    logPath = get_dir_to_module();
    append_to_path(logPath, "NPClient.log");
  }
  return logPath;
}

std::fstream& get_log_stream()
{
  static auto stream = std::fstream(get_log_path(), std::ios::out | std::ios::trunc);
  return stream;
} 

} //logging
