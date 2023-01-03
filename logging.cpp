#include "logging.hpp"

std::string get_log_path()
{
  auto path = get_dir_to_module();
  append_to_path(path, "NPClient.log");
  return path;
}

std::fstream& get_log_stream()
{
  static auto stream = std::fstream(get_log_path(), std::ios::out | std::ios::trunc);
  return stream;
} 
