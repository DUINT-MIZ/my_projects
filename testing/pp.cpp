#include "static_parser.hpp"
#include <array>
#include <frozen/unordered_map.h>
#include <frozen/string.h>

constexpr ProfileTable<2, 0> table(
    ConstructingProfile(false)
    .long_name("--help")
    .short_name("-h")
    .expected(0)
    .restricted()
    .immediate(),
    ConstructingProfile(false)
    .long_name("--int")
    .short_name("-i")
    .expected(1)
    .convert_to(TypeCode::DOUBLE)
);

constexpr auto smapper = make_mapper([]() -> const auto& { return table; });

void show_help(static_profile _, modifiable_profile& __) {
    std::cout << "Help message !" << std::endl;
    std::exit(0);
}

void int_called(static_profile _, modifiable_profile& __) {
    std::cout << "Deint was called !" << std::endl;
}

int main(int argc, const char* argv[]) {
    
    std::array<Blob, 10> int_buff{};
    std::array<modifiable_profile, 2> mod_profs{
        modifiable_profile()
        .set_callback(show_help),
        modifiable_profile()
        .bind(pointing_arr(int_buff))
        .set_callback(int_called)
    };

    RuntimeMapper<4> rmapper(smapper, mod_profs);
    
    parse(rmapper, ++argv, --argc, DumpSize<0>{});
    std::cout << "int val : ";
    for(auto& blob : int_buff) {
        std::cout << *reinterpret_cast<double*>(blob.data) << ", ";
    }
    std::cout << std::endl;
}   