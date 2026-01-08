#include <cstdint>
#include <stdexcept>
#include <span>
#include <utility>
#include <frozen/unordered_map.h>
#include <frozen/string.h>
#include <array>

#ifndef STATIC_PARSER_NO_HEAP
#include <string>
#endif

#include "values.hpp"
#include "utils.hpp"
#include "exceptions.hpp"
#include "profiles.hpp"

// Note : Object validity are it's own responsibility

template <std::size_t N, std::size_t PosargCount>
class RuleSet {
    private :
    std::array<const static_profile*, PosargCount> posargs{};
    std::size_t existing_posarg = 0;
    
    public :
    const std::array<static_profile, N> rules;
    RuleSet() = delete;

    template <std::size_t... Is>
    constexpr RuleSet(const std::array<ConstructingProfile, N>& raw_rule, std::index_sequence<Is...>)
    : rules({ raw_rule[Is]... })
    {
        for(const auto& prof : rules) {
            if(prof.posarg) {
                if(prof.positional_order >= PosargCount)
                    throw comtime_except("Positional order out of bounds");
                if(posargs[prof.positional_order])
                    throw comtime_except("Positional order occupied by other posarg");
                posargs[prof.positional_order] = &prof;
                ++existing_posarg;
            }
        } 

        auto anchor = posargs.end();
        auto it = posargs.begin();
        for(; it != posargs.end(); it++) {
            if(*it == nullptr) {
                anchor = it;
                break;
            }
        }

        // Ambiguous !
        if(not(
            ((it - posargs.begin()) != existing_posarg) and
            (it != posargs.end())
        )) return;


        for(; it != posargs.end(); it++) {
            if(*it != nullptr) {
                *anchor = *it;
                *it = nullptr;
                ++anchor;
            }
        }
    }

    constexpr const std::array<const static_profile*, PosargCount>& posarg_order_get() const noexcept { return posargs; }
    constexpr std::size_t get_existing_posarg() const noexcept { return existing_posarg; }
};

template <std::size_t IDCount>
class StaticMapper { // Handle everything to retrieve a profile data
    using MapType = frozen::unordered_map<frozen::string, const static_profile*, IDCount>;

    private :

    std::span<const static_profile*> posargs;
    MapType map;
    std::span<const static_profile> profiles;

    void verify_name(const static_profile* target, NameType name) {
        auto it = map.find(name);
        if(it == map.end()) 
            throw comtime_except("Can't find name in map (forget to register ?)");
        if(target != it->second)
            throw comtime_except("Name points to the wrong profile in map (switched up ?)");
    }

    public :

    StaticMapper() = delete;
    template <std::size_t N, std::size_t PosargCount>
    constexpr StaticMapper(
        const MapType& new_map,
        const RuleSet<N, PosargCount>& rule_set
    ) : map(new_map),
        profiles(rule_set.rule_get()),
        posargs(rule_set.posarg_order_get())
    {
        posargs = posargs.subspan(0, rule_set.get_existing_posarg());
        std::size_t valid_mappings = 0;
        for(const auto& prof : profiles) {
            if(prof.lname) {
                verify_name(&prof, prof.lname);
                ++valid_mappings;
            }

            if(prof.sname) {
                verify_name(&prof, prof.sname);
                ++valid_mappings;
            }

            if(prof.convert_code == TypeCode::ARRAY)
                comtime_except("ARRAY is not a specific type conversion code");
        }
        if(valid_mappings != IDCount)
            throw comtime_except("Unknown name was assigned to map");
    }

    constexpr const static_profile* get_prof(NameType name) const noexcept {
        auto it = map.find(name);
        if(it != map.end()) return &it->second;
        return nullptr;
    }

    constexpr const static_profile* get_posarg(std::size_t order) const noexcept {
        if(order >= posargs.size()) return nullptr;
        return posargs[order];
    }

    constexpr const std::size_t count_distance(const static_profile* ptr) const noexcept {
        return (ptr - &profiles[0]);
    }

    constexpr const auto& get_profiles() const noexcept { return profiles; }
    constexpr const auto& get_posarg_order() const noexcept { return posargs; }
    constexpr const auto& get_map() const noexcept { return map; }
};

using FindPair = std::pair<const static_profile*, modifiable_profile*>;

template <std::size_t IDCount>
class RuntimeMapper { // Same as StaticMapper, + find a modifiable_profile pair


    private :
    std::span<modifiable_profile> mutable_profiles;
    const StaticMapper<IDCount>& smap; // static mapper

    public :
    
    
    RuntimeMapper(
        const StaticMapper<IDCount>& new_smap,
        std::span<modifiable_profile> mprof // Just short for mutable_prof
    ) : smap(new_smap), mutable_profiles(std::move(mprof))
    {
        if(mutable_profiles.size()  != smap.existed_profiles())
            #ifdef STATIC_PARSER_NO_HEAP
            throw SetupError("mutable_profiles size does not match with StaticMapper existing profiles");
            #else
            throw SetupError(
                "Mutable profile size of " + std::to_string(mutable_profiles.size())
                + ", Does not match StaticMapper existing profiles of " + std::to_string(smap.existed_profiles())
            );
            #endif
        // TODO
        const auto& profiles = smap.get_profiles();
        for(std::size_t i = 0; i < profiles.size(); i++) {
            
        }
    }

    FindPair get_pair(NameType name) noexcept {
        const static_profile* prof = smap.get_prof(name);
        if(!prof) return {nullptr, nullptr};
        return {prof, &mutable_profiles[smap.count_distance(prof)]};
    }

    FindPair get_posarg_pair(std::size_t order) noexcept {
        const static_profile* prof = smap.get_posarg(order);
        if(!prof) return {nullptr, nullptr};
        return {prof, &mutable_profiles[smap.count_distance(prof)]};
    }

    std::size_t existing_posarg() const noexcept { return smap.get_posarg().size(); }
    std::size_t existing_profiles() const noexcept { return smap.get_profiles().size(); }
};

constexpr bool potential_digit(const std::string_view& str) noexcept {
    switch(str[0]) {
        case '+' : case '-' :
        return std::isdigit(str[1]);

        default : return std::isdigit(str[0]);
    }
}

void convert_and_insert(const char* val, BoundValue& bval)

template <typename ArgGetF>
std::string_view fetch_and_next(const ArgGetF& get, FindPair& complete_prof, const char* eq_value) {
    std::size_t to_parse = (complete_prof.first->narg - complete_prof.second->fulfilled_args);
    if((((signed)to_parse) <= 0) and is_restricted(complete_prof.first->behave))
        return get();

    if(eq_value) {

    }

}


template <typename DumpStoreF, typename ArgGetF, std::size_t IDCount>
void handle_opt(
    RuntimeMapper<IDCount>& rmap,
    const ArgGetF& get,
    DumpStoreF store
) {
    std::string_view curr_token = get();
    std::string_view eq_value;
    std::size_t eq_idx;

    while(!curr_token.empty()) {

        if((curr_token[0] != '-') or potential_digit(curr_token))
            store(curr_token);

        FindPair complete_prof = rmap.get_prof(curr_token);
        
        if(!complete_prof.first or !complete_prof.second)
            #ifdef STATIC_PARSER_NO_HEAP
            throw ParseError("Unknown flag is passed");
            #else
            throw ParseError((std::string("Flag of \'") + curr_token) + "\' is unknown");
            #endif

        if((eq_idx = curr_token.find('=')) != std::string_view::npos) {
            eq_value = curr_token.substr(eq_idx, curr_token.size() - (curr_token + 1));
            curr_token = curr_token.substr(0, eq_idx);
        }

        curr_token = fetch_and_next(get, complete_prof, eq_value.data());

        if(!eq_value.empty())
            eq_value = std::string_view{};
    }
}