#include "ui.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <vector>

struct Item{std::filesystem::path path; uintmax_t size;};

void print_top(const std::map<std::filesystem::path,uintmax_t>& sizes, size_t n, bool json){
    std::vector<Item> items;
    for(auto& [p,s]: sizes) items.push_back({p,s});
    std::sort(items.begin(), items.end(), [](const Item&a,const Item&b){return a.size>b.size;});
    if(json){
        std::cout<<"[";
        for(size_t i=0;i<std::min(n,items.size());++i){
            if(i) std::cout<<",";
            std::cout<<"{\"path\":\""<<items[i].path.string()<<"\",\"size\":"<<items[i].size<<"}";
        }
        std::cout<<"]"<<std::endl;
    }else{
        for(size_t i=0;i<std::min(n,items.size());++i){
            std::cout<<std::setw(10)<<items[i].size/1024/1024<<" MB\t"<<items[i].path.string()<<"\n";
        }
    }
}
