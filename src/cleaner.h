#pragma once
#include <filesystem>
#include <vector>
struct CleanOptions{bool dry_run=false;};
std::vector<std::filesystem::path> default_candidate_paths();
uintmax_t clean_paths(const std::vector<std::filesystem::path>& paths,const CleanOptions& opt);
