#pragma once


#include "exceptions.hpp"
#include "utils.hpp"
#include "values.hpp"
#include "profiles.hpp"
#include "mapper.hpp"
#include "parser.hpp"

namespace sp {

using namespace sp;

template <std::size_t IDCount, std::size_t... Is>
constexpr auto
make_map_pairs(
    std::array<std::pair<profiles::NameType, const profiles::static_profile*>, IDCount>& extracted,
    std::index_sequence<Is...>
) {
    return std::array<std::pair<frozen::string, const profiles::static_profile*>, IDCount>
    {{
        {frozen::string(extracted[Is].first), extracted[Is].second}...
    }};
}

template <std::size_t IDCount>
constexpr
frozen::unordered_map<frozen::string, const profiles::static_profile*, IDCount>
make_map(const std::span<const profiles::static_profile>& profiles) { 
    std::array<std::pair<profiles::NameType, const profiles::static_profile*>, IDCount> extracted{};
    std::size_t curr_idx = 0;
    for(const auto& prof : profiles) {
        if(prof.lname) extracted[curr_idx++] = {prof.lname, &prof};
        if(prof.sname) extracted[curr_idx++] = {prof.sname, &prof};
    }

    return 
    frozen::make_unordered_map<frozen::string, const profiles::static_profile*>(
        make_map_pairs(extracted, std::make_index_sequence<IDCount>{})
    );
}

template <std::size_t IDCount, std::size_t ProfCount, std::size_t PosargCount>
struct Context {
    static constexpr std::size_t id_count = IDCount;
    static constexpr std::size_t prof_count = ProfCount;
    static constexpr std::size_t posarg_count = PosargCount;
    mapper::ProfileTable<ProfCount, PosargCount> ptable;
    mapper::Mapper<IDCount> mapper;
    template <profiles::DenotedProfile... Prof>
    constexpr Context(const Prof&... prof)
    : ptable(prof...),
      mapper(make_map<IDCount>(ptable.static_profiles), ptable)
    {}

    profiles::modifiable_profile& match(std::span<profiles::modifiable_profile> mprof, const profiles::NameType& name) const {
        const profiles::static_profile* prof = mapper[name];
        if(!prof) 
            throw std::invalid_argument("Unknown name");
        std::size_t index = ptable.profile_index(prof);
        if(index >= mprof.size())
            throw std::invalid_argument("mprof is out of range");
        return mprof[index];
    }
};

template <profiles::DenotedProfile... Prof>
constexpr std::size_t count_posargs() noexcept {
    std::size_t dummy = 0;
    std::size_t posarg_count = 0;
    ((Prof::is_posarg_type::value ? ++posarg_count : ++dummy), ...);
    return posarg_count;
}

template <profiles::DenotedProfile... Prof>
constexpr auto make_context(const Prof&... prof) {
    constexpr std::size_t ids = (Prof::id_count + ...);
    constexpr std::size_t posarg_count = count_posargs<Prof...>();
    constexpr std::size_t profile_count = sizeof...(Prof);

    return Context<ids, profile_count, posarg_count>(prof...);
}

}