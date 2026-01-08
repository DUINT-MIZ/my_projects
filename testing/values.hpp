#pragma once

#include <cstdint>
#include <array>
#include <iostream>
#include <variant>
#include <type_traits>
#include <span>

enum class TypeCode {
	INT,
	STRING,
	DOUBLE,
	ARRAY,
	NONE
};

const char* code_to_str(TypeCode code) noexcept {
	switch(code){
		case TypeCode::INT : return "<INT>";
		case TypeCode::STRING : return "<STRING>";
		case TypeCode::DOUBLE : return "<DOUBLE>";
		case TypeCode::ARRAY : return "<ARRAY>";
		case TypeCode::NONE : return "<NONE>";
		default :
			return "<UNKNOWN>";
	}
}

template <typename... T>
constexpr std::size_t maximum() {
	std::array<std::size_t, sizeof...(T)> sizes = { sizeof(T)... };
	std::size_t res = 0;
	for(auto type_size : sizes) { if(type_size > res) res = type_size; }
	return res;
}

template <typename T>
struct unwrap_reference {using type = T; };

template <typename T>
struct unwrap_reference<std::reference_wrapper<T>> { using type = T; };

using IntT = int;
using DobT = double;
using StrT = const char*;
using Blob = alignas(double) char[maximum<DobT, IntT, StrT>()];

using IntRef = std::reference_wrapper<IntT>;
using DobRef = std::reference_wrapper<DobT>;
using StrRef = std::reference_wrapper<StrT>;
using ArrT = std::span<Blob>;

struct pointing_arr {
	ArrT viewer;
	std::size_t curr_idx = 0;
	pointing_arr(const ArrT& view) : viewer(view) {}
}

class BoundValue {
	private :
	using ValueType = std::variant<
		std::monostate,
		IntRef,
		DobRef,
		StrRef,
		pointing_arr
	>;

	ValueType value;

	static bool fill_fail(void* _, BoundValue& __, TypeCode ___) { return false; }

	static bool fill_int(void* val, BoundValue& ins, TypeCode _) {
		std::get<IntRef>(ins.value).get() =
			*reinterpret_cast<IntT*>(val);
		return true;
	}

	static bool fill_dob(void* val, BoundValue& ins, TypeCode _) {
		std::get<DobRef>(ins.value).get() =
			*reinterpret_cast<DobT*>(val);
		return true;
	}

	static bool fill_str(void* val, BoundValue& ins, TypeCode _) {
		std::get<StrRef>(ins.value).get() =
			*reinterpret_cast<StrT*>(val);
		return true;
	}

	static bool fill_arr(void* val, BoundValue& ins, TypeCode code) {
		pointing_arr& arr = std::get<pointing_arr>(ins.value);
		if(arr.curr_idx >= arr.viewer.size()) return false;
		switch(code) {
		case TypeCode::INT :
			*reinterpret_cast<IntT>(arr.viewer[arr.curr_idx]) = 
				*reinterpret_cast<IntT*>(val);
			break;

		case TypeCode::DOUBLE :
			*reinterpret_cast<DobT>(arr.viewer[arr.curr_idx]) = 
				*reinterpret_cast<DobT*>(val);
			break;

		case TypeCode::STRING :
			*reinterpret_cast<StrT>(arr.viewer[arr.curr_idx]) =
				*reinterpret_cast<StrT*>(val);
			break;

		default : return false;
		}
		++arr.curr_idx;
		return true;
	}
	
	bool (*fill_method)(void*, BoundValue&, TypeCode) = fill_fail;

	public :


	BoundValue() = default;

	void bind(int& var) {
		this->value = var;
		this->fill_method = fill_int;
	}

	void bind(double& var) {
		this->value = var;
		this->fill_method = fill_dob;
	}

	void bind(const char*& var) {
		this->value = var;
		this->fill_method = fill_str;
	}

	void bind(const pointing_arr& var) {
		this->value = var;
		this->fill_method = fill_arr;
	}

	bool fill(void* val, TypeCode code) { return fill_method(val, *this, code); }

	template <typename RefWrapperT>
	unwrap_reference<RefWrapperT>::type& get() {
		return std::get<RefWrapperT>(value).get();
	}

	ArrT get_array() {
		return std::get<pointing_arr>(value).viewer;
	}

	TypeCode get_code() {
		return std::visit(
			[](auto&& args) {
				using T = std::decay_t<decltype(args)>;
				if constexpr (std::is_same_v<T, IntRef>) return TypeCode::INT;
				if constexpr (std::is_same_v<T, DobRef>) return TypeCode::DOUBLE;
				if constexpr (std::is_same_v<T, StrRef>) return TypeCode::STRING;
				if constexpr (std::is_same_v<T, ArrT>) return TypeCode::ARRAY;
				return TypeCode::NONE;
			},
			value
		);
	}
};

