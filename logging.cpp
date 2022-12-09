#include "logging.hpp"

std::string get_log_path()
{
  return "NPClient.log";
}

std::fstream& get_log_stream()
{
  static auto stream = std::fstream(get_log_path(), std::ios::out | std::ios::trunc);
  return stream;
} 
