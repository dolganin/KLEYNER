#include <gtest/gtest.h>
#include <fstream>
#include "scanner.h"

TEST(Scanner, SizeCalc){
    auto tmp = std::filesystem::temp_directory_path()/"scanner_test";
    std::filesystem::create_directories(tmp/"sub");
    std::ofstream(tmp/"a.txt")<<"hello";
    std::ofstream(tmp/"sub"/"b.txt")<<"world";
    auto res = scan_directory_sizes(tmp,{});
    ASSERT_GT(res[tmp],0u);
    std::filesystem::remove_all(tmp);
}
