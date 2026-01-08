#pragma once
#include "exceptions.hpp"
#include "values.hpp"
#include "utils.hpp"
#include <cstdint>

#ifndef STATIC_PARSER_NO_HEAP
#include <functional>
#endif


using FlagType = std::uint32_t;

static constexpr FlagType kRequired = 1 << 0;
static constexpr FlagType kRestricted = 1 << 1;
static constexpr FlagType kImmediate = 1 << 2;

constexpr bool is_required(FlagType flag) { return ((flag & kRequired) != 0); }
constexpr bool is_restricted(FlagType flag) { return ((flag & kRestricted) != 0); }
constexpr bool is_immediate(FlagType flag) { return ((flag & kImmediate) != 0); }

using NameType = const char*;
using WholeNumT = std::uint32_t;
using NumT = std::int32_t;
struct static_profile;

struct ConstructingProfile {
    private :
    NameType lname = nullptr;
    NameType sname = nullptr;
    WholeNumT positional_order = 0; // Actual order is positional_order - 1
    WholeNumT narg = 0;
    NumT exclude_point = -1;
    WholeNumT call_limit = 1;
    FlagType behave = 0;
    TypeCode convert_code = TypeCode::NONE;
    bool posarg = false;

    constexpr void verify() const {
        if(!lname and !sname)
            throw comtime_except("Empty name is forbidden");

        if(posarg) {
            if(sname) 
                throw comtime_except("Posarg shuldn't not have a short name");
            if(!lname)
                throw comtime_except("Empty long name are forbidden on posarg");
            if(!narg)
                throw comtime_except("Empty narg are forbidden on posarg");
        }

        if(narg and (convert_code == TypeCode::NONE))
            throw comtime_except("Convert code of NONE with narg of non-0 are forbidden");

        if(!call_limit)
            throw comtime_except("Call limit of 0 are forbidden");
    }

    friend static_profile;

    public :

    constexpr ConstructingProfile(bool is_a_posarg) : posarg(is_a_posarg) {}

    constexpr ConstructingProfile& required() noexcept {
        behave |= kRequired;
        return *this;
    }

    constexpr ConstructingProfile& immediate() noexcept {
        behave |= kImmediate;
        return *this;
    }

    constexpr ConstructingProfile& restricted() noexcept {
        behave |= kRestricted;
        return *this;
    }

    constexpr ConstructingProfile& order(WholeNumT pos) noexcept {
        positional_order = pos;
        return *this;
    }

    constexpr ConstructingProfile& long_name(NameType name) noexcept {
        lname = name;
        return *this;
    }

    constexpr ConstructingProfile& short_name(NameType name) noexcept {
        sname = name;
        return *this;
    }

    constexpr ConstructingProfile& expected(WholeNumT new_narg) noexcept {
        narg = new_narg;
        return *this;
    }

    constexpr ConstructingProfile& limit_call(WholeNumT lim) noexcept {
        call_limit = lim;
        return *this;
    }

    constexpr ConstructingProfile& convert_to(TypeCode code) noexcept {
        convert_code = code;
        return *this;
    }

    constexpr NameType get_lname() const noexcept { return lname; }
    constexpr NameType get_sname() const noexcept { return sname; }
    constexpr WholeNumT expectations() const noexcept { return narg; }
    constexpr WholeNumT pos_order() const noexcept { return positional_order; }
    constexpr WholeNumT limit_of_call() const noexcept { return call_limit; }
    constexpr bool is_a_posarg() const noexcept { return posarg; }
    constexpr int exclusion_point() const noexcept { return exclude_point; }
    constexpr FlagType behavior() const noexcept { return behave; }
    constexpr TypeCode conversion_code() const noexcept { return convert_code; }
};

struct static_profile {
    public :
    const NameType lname = nullptr;
    const NameType sname = nullptr;
    const WholeNumT call_limit = 1;
    const WholeNumT narg = 0;
    const WholeNumT positional_order = 0;
    const FlagType behave = 0;
    const NumT exclude_point = -1;
    const TypeCode convert_code = TypeCode::NONE;
    const bool posarg = false;

    static_profile() = delete;
    constexpr static_profile(const ConstructingProfile& construct_prof)
    :   posarg(construct_prof.is_a_posarg()),
        lname(construct_prof.get_lname()),
        sname(construct_prof.get_sname()),  
        narg(construct_prof.expectations()),
        call_limit(construct_prof.limit_of_call()),
        exclude_point(construct_prof.exclusion_point()),
        behave(construct_prof.behavior()),
        convert_code(construct_prof.conversion_code()),
        positional_order(construct_prof.pos_order()) 
    { construct_prof.verify(); }

    constexpr static_profile(const static_profile& oth) = default;
};
struct modifiable_profile {
    WholeNumT call_count = 0;
    WholeNumT fulfilled_args = 0;
    #ifdef STATIC_PARSER_NO_HEAP
    using FunctionType = void(*)(static_profile, modifiable_profile&);
    #else
    using FunctionType = std::function<void(static_profile, modifiable_profile&)>;
    #endif
    FunctionType callback = [](static_profile, modifiable_profile&){};
    BoundValue bval;
    WholeNumT call_frequent() const noexcept { return call_count; }
    template <typename T>
    modifiable_profile& bind(T& var) { val.bind(var); return *this; }
    modifiable_profile& set_callback(FunctionType&& func) { callback = func; return *this; }
};

const char* get_name(const static_profile& prof) {
    return (prof.lname ? prof.lname : prof.sname);
}