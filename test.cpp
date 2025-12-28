#include "include/bash_utils/wording.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
    if(argc <= 1) {
        std::cout << "No input..." << std::endl;
        return 1;
    }
    const char* suggestions[] = {
        "abide",
        "abore",
        "abort",
        "about"
    };

    auto res = wording::suggestion_make_tree(&suggestions[0], &suggestions[3]);
    std::cerr << "root pointer : " << (void*)res.root << std::endl;
    wording::print_suggestion(res.root, argv[1]);
    return 0;
}