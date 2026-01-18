#include "static_parser.hpp"
#include <array>
#include <frozen/unordered_map.h>
#include <frozen/string.h>
#include <variant>

template <std::size_t IDCount, std::size_t ProfCount, std::size_t PosargCount>
struct Context {
    static constexpr std::size_t id_count = IDCount;
    static constexpr std::size_t prof_count = ProfCount;
    static constexpr std::size_t posarg_count = PosargCount;
    ProfileTable<ProfCount, PosargCount> ptable;
    Mapper<IDCount> mapper;
    template <DenotedProfile... Prof>
    constexpr Context(const Prof&... prof)
    : ptable(prof...),
      mapper(make_map<IDCount>(ptable.static_profiles), ptable)
    {}
};

template <DenotedProfile... Prof>
constexpr std::size_t count_posargs() noexcept {
    std::size_t dummy = 0;
    std::size_t posarg_count = 0;
    ((Prof::is_posarg_type::value ? ++posarg_count : ++dummy), ...);
    return posarg_count;
}

template <DenotedProfile... Prof>
constexpr auto make_context(const Prof&... prof) {
    constexpr std::size_t ids = (Prof::id_count + ...);
    constexpr std::size_t posarg_count = count_posargs<Prof...>();
    constexpr std::size_t profile_count = sizeof...(Prof);

    return Context<ids, profile_count, posarg_count>(prof...);
}


constexpr auto context = make_context(
    snOption()["-h"].nargs(0).convert(TypeCode::NONE)
);

int main(int argc, const char* argv[]) {
    std::cout << "size : " << context.mapper.profiles.size() << std::endl;
}   