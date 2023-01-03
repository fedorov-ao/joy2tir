#include "logging.hpp"
#include <cstdlib>

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
