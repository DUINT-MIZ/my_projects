#pragma once
#include <cstdint>
#include <array>
#include <span>
#include <utility>
#include <frozen/unordered_map.h>
#include <frozen/string.h>

#include "exceptions.hpp"
#include "profiles.hpp"

template <typename T>
concept IsProfile = std::is_same_v<std::decay_t<T>, ConstructingProfile>;

template <std::size_t ProfCount, std::size_t PosargCount>
class ProfileTable {
    private :
    std::array<const static_profile*, PosargCount> posargs{};
    
    public :
    const std::array<static_profile, ProfCount> static_profiles;
    ProfileTable() = delete;
    template<IsProfile... Prof>
    constexpr ProfileTable(Prof&&... raw_rule)
    : static_profiles({ raw_rule... })
    {
        std::size_t curr_posarg_i = 0;
        std::size_t existing_posarg = 0;
        const static_profile** spot = nullptr;

        for(const auto& prof : static_profiles) {
            if(prof.is_posarg) {
                if(existing_posarg >= PosargCount)
                    throw comtime_except("Existing posarg exceed template argument PosargCount");

                if(prof.positional_order < 0)
                    spot = &posargs[curr_posarg_i++];
                else if(prof.positional_order < PosargCount)
                    spot = &posargs[prof.positional_order];
                else
                    throw comtime_except("Posarg positional order is out of template argument PosargCount reach");

                if(!(*spot))
                    *spot = &prof;
                else
                    throw comtime_except("Posarg positional order is occupied by another posarg");
                ++existing_posarg;
            }
        }

        if(existing_posarg < PosargCount)
            throw comtime_except("Existing posarg doesn't match template argument PosargCount");
    }

    constexpr const std::array<const static_profile*, PosargCount>& get_posargs() const noexcept { return posargs; }
};

struct PosargIndex {
    std::size_t val = 0;
    PosargIndex(std::size_t i) : val(i) {}
};

template <std::size_t IDCount>
class Mapper {
    private :
    using MapType = frozen::unordered_map<frozen::string, const static_profile*, IDCount>;
    
    constexpr void verify_connection(const static_profile* target, NameType name) {
        auto it = map.find(frozen::string(name));
        if(it == map.end()) 
            throw comtime_except("Unknown profile name in map (Forget to register ?)");
        if(it->second != target)
            throw comtime_except("Name in map, points to the wrong profile");
    }

    template <std::size_t N>
    constexpr std::span<const static_profile*> get_ptable_posarg(const std::array<const static_profile*, N>& arr)
    {
        if constexpr  (N == 0) { 
            return {};
        } else {
            return std::span<const static_profile*>(arr);
        }
    }

    public :
    const MapType map;
    const std::span<const static_profile> profiles;
    const std::span<const static_profile*> posargs;

    template <std::size_t ProfCount, std::size_t PosargCount>
    constexpr Mapper(
        const MapType& new_map,
        const ProfileTable<ProfCount, PosargCount>& ptable
    ) : map(new_map), profiles(ptable.static_profiles), posargs(get_ptable_posarg(ptable.get_posargs()))
    {
        std::size_t valid_mappings = 0;
        for(const auto& prof : profiles) {
            if(prof.lname) {
                verify_connection(&prof, prof.lname);
                ++valid_mappings;
            }

            if(prof.sname) {
                verify_connection(&prof, prof.sname);
                ++valid_mappings;
            }
        }

        if(valid_mappings < IDCount)
            throw comtime_except("Unknown name was assigned to the map");
    }

    const static_profile* operator[](std::size_t idx) const noexcept {
        if(idx >= profiles.size()) return nullptr;
        return &profiles[idx];
    }

    const static_profile* operator[](const PosargIndex& posarg_index) const noexcept {
        if(posarg_index.val >= posargs.size()) return nullptr;
        return posargs[posarg_index.val];
    }

    const static_profile* operator[](const std::string_view& name) const noexcept {
        auto it = map.find(frozen::string(name));
        if(it == map.end()) return nullptr;
        return it->second;
    }

    std::size_t profile_index(const static_profile* target) const noexcept {
        return (target - &profiles[0]);
    }
};

using FindPair = std::pair<const static_profile*, modifiable_profile*>;

template <std::size_t IDCount>
class RuntimeMapper {
    private :
    std::span<modifiable_profile> mutable_profiles;
    public :
    const Mapper<IDCount>& mapper; // const reference in case mapper is compile-time evaluated object

    RuntimeMapper(
        const Mapper<IDCount>& new_mapper,
        const std::span<modifiable_profile> new_mutable_profiles
    ) : mutable_profiles(new_mutable_profiles), mapper(new_mapper) 
    {
        if(mutable_profiles.size() != mapper.profiles.size())
            throw std::invalid_argument("mutable profile size doesn't match mapper profile size");
        std::size_t lim = mapper.profiles.size();
        for(std::size_t i = 0; i < lim; i++) {
            const static_profile& sprof = *mapper[i];
            modifiable_profile& mprof = mutable_profiles[i];

            if(mprof.bval.get_code() != TypeCode::ARRAY) {
                if(mprof.bval.get_code() != sprof.convert_code)
                    throw std::invalid_argument("BoundValue variable reference type is incompatible with static_profile convert code");
                if(sprof.narg > 1)
                    throw std::invalid_argument("static_profile narg more than 1 is incompatible with variable reference BoundValue");
            } else {
                if(mprof.bval.get_array().viewer.size() < sprof.narg)
                    throw std::invalid_argument("BoundValue array size is less than static_profile narg");
            }
        }
    }

    FindPair operator[](std::size_t idx) {
        const static_profile* prof = mapper[idx];
        if(!prof) return {nullptr, nullptr};
        return {prof, &mutable_profiles[idx]};
    }

    FindPair operator[](const PosargIndex& posarg_index) {
        const static_profile* prof = mapper[posarg_index];
        if(!prof) return {nullptr, nullptr};
        return {prof, &mutable_profiles[mapper.profile_index(prof)]};
    }

    FindPair operator[](const std::string_view& name) {
        const static_profile* prof = mapper[name];
        if(!prof) return {nullptr, nullptr};
        return {prof, &mutable_profiles[mapper.profile_index(prof)]};
    }

    std::size_t existing_profile() const noexcept {
        return mapper.profiles.size();
    }

    std::size_t existing_posarg() const noexcept {
        return mapper.posargs.size();
    }
};

template <std::size_t N>
constexpr std::size_t get_size(const frozen::unordered_map<frozen::string, const static_profile*, N>& _)
{
    return N;
}

template <std::size_t N>
constexpr std::size_t count_ids(const std::array<static_profile, N>&  profiles) {
    std::size_t ids = 0;
    for(const auto& prof : profiles) {
        if(prof.lname) ++ids;
        if(prof.sname) ++ids;
    }
    return ids;
}


template <std::size_t N, std::size_t... Is>
constexpr auto make_map_pairs(
    const std::array<std::pair<NameType, const static_profile*>, N>& arr,
    std::index_sequence<Is...>
) {
    return std::array<std::pair<frozen::string, const static_profile*>, N>({
        {frozen::string(arr[Is].first), arr[Is].second}...
    });
}

template <typename PtableGetF>
constexpr auto make_map(PtableGetF ptable_get) {
    constexpr auto& ptable = ptable_get();
    constexpr std::size_t ids = count_ids(ptable.static_profiles);
    std::array<std::pair<NameType, const static_profile*>, ids> pairs{};
    std::size_t i = 0;
    for(const auto& prof : ptable.static_profiles) {
        if(prof.lname) pairs[i++] = {prof.lname, &prof};
        if(prof.sname) pairs[i++] = {prof.sname, &prof};
    }
    return frozen::make_unordered_map<frozen::string, const static_profile*>(make_map_pairs(pairs, std::make_index_sequence<ids>{}));
}

template <typename PtableGetF>
constexpr auto make_mapper(PtableGetF ptable_get) {
    constexpr auto map = make_map(ptable_get);
    return Mapper<get_size(map)>(map, ptable_get());
}