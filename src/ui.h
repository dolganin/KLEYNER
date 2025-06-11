#pragma once
#include <map>
#include <filesystem>
#include <vector>
void print_top(const std::map<std::filesystem::path,uintmax_t>& sizes, size_t n, bool json);
