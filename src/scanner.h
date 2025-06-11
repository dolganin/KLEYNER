#pragma once
#include <filesystem>
#include <map>
#include <vector>

std::map<std::filesystem::path, uintmax_t>
scan_directory_sizes(const std::filesystem::path& root,
                     const std::vector<std::filesystem::path>& exclude);
