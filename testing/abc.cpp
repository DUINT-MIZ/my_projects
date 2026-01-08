#include "profiles.hpp"
#include <iostream>

constexpr static_profile prof(
    ConstructingProfile(true)
    .expected(3)
    .long_name("files")
    .convert_to(TypeCode::INT)
);

int main() {
    std::cout << prof.narg << std::endl;
}