#pragma once
#include <concepts>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include <variant>
#include <optional>
#include <vector>
#include <array>

namespace zs
{
	template<typename T>
	concept POD = std::is_pod_v<T>;

	template<typename T>
	constexpr bool String_ = false;
	template<typename T>
	constexpr bool String_<std::basic_string<T>> = true;
	template<typename T>
	concept String = String_<T>;

	template<typename T>
	constexpr bool StringView_ = false;
	template<typename T>
	constexpr bool StringView_<std::basic_string_view<T>> = true;
	template<typename T>
	concept StringView = StringView_<T>;

	template<typename T>
	constexpr bool Optional_ = false;
	template<typename T>
	constexpr bool Optional_<std::optional<T>> = true;
	template<typename T>
	concept Optional = Optional_<T>;

	template<typename T>
	constexpr bool Vector_ = false;
	template<typename T>
	constexpr bool Vector_<std::vector<T>> = true;
	template<typename T>
	concept Vector = Vector_<T>;

	template<typename T>
	constexpr bool Array_ = false;
	template<typename T, size_t size>
	constexpr bool Array_<std::array<T, size>> = true;
	template<typename T>
	concept Array = Array_<T>;

	template<typename T, typename U>
	concept Same = std::is_same_v<T, U>;

	void Write(std::ostream& os, const void* source, size_t bytes)
	{
		os.write(reinterpret_cast<const char*>(source), bytes);
	}

	template<typename T>
	void Write(std::ostream& os, const T& value);

	template<typename T>
	void Write(std::ostream& os, const T* value) =delete;

	template<POD T>
	void Write(std::ostream& os, const T& value)
	{
		Write(os, std::addressof(value), sizeof(value));
	}

	template<Optional T>
	void Write(std::ostream& os, const T& value)
	{
		if (value)
		{
			Write(os, true);
			Write(os, *value);
		}
		else
		{
			Write(os, false);
		}
	}

	template<typename T>
		requires (Vector<T>&& POD<typename T::value_type>) || String<T> || StringView<T>
	void Write(std::ostream& os, const T& value)
	{
		Write(os, value.size());
		Write(os, value.data(), value.size() * sizeof(T::value_type));
	}

	template<typename T> requires !POD<T>
	void Write(std::ostream& os, const std::vector<T>& vec)
	{
		Write(os, vec.size());
		for (const auto& v : vec)
			Write(os, v);
	}

	template<typename T, size_t size> requires !POD<T>
	void Write(std::ostream& os, const std::array<T, size>& arr)
	{
		for (const auto& v : arr)
			Write(os, v);
	}

	bool Read(std::istream& is, void* dest, size_t bytes)
	{
		is.read(reinterpret_cast<char*>(dest), bytes);
		return is.gcount() == bytes;
	}

	struct Error {};

	template<typename T>
	std::variant<T, Error> Read(std::istream& is);

	template<POD T>
	std::variant<T, Error> Read(std::istream& is)
	{
		T value;
		if (Read(is, std::addressof(value), sizeof(value)))
			return value;
		else
			return Error{};
	}

	template<Optional T>
	std::variant<T, Error> Read(std::istream& is)
	{
		auto hasValue = Read<bool>(is);
		if (std::holds_alternative<Error>(hasValue))
			return Error{};

		if (std::get<bool>(hasValue))
		{
			auto value = Read<T::value_type>(is);
			if (std::holds_alternative<Error>(value))
				return Error{};
			return T(std::get<T::value_type>(value));
		}
		else
		{
			return std::nullopt;
		}
	}

	template<typename T>
		requires (Vector<T>&& POD<typename T::value_type>) || String<T>
	std::variant<T, Error> Read(std::istream& is)
	{
		auto size = Read<size_t>(is);
		if (std::holds_alternative<Error>(size))
			return Error{};

		T value;
		value.resize(std::get<size_t>(size));
		if (Read(is, value.data(), value.size() * sizeof(T::value_type)))
			return value;
		else
			return Error{};
	}

	template<typename T> requires Vector<T> && !POD<typename T::value_type>
	std::variant<T, Error> Read(std::istream& is)
	{
		auto size = Read<size_t>(is);
		if (std::holds_alternative<Error>(size))
			return Error{};

		T vec;
		for (size_t i = 0;i < std::get<size_t>(size);++i)
		{
			auto v = Read<T::value_type>(is);
			if (std::holds_alternative<Error>(v))
				return Error{};
			vec.emplace_back(std::move(std::get<T::value_type>(v)));
		}
		return vec;
	}

	template<typename T> requires Array<T> && !POD<typename T::value_type>
	std::variant<T, Error> Read(std::istream& is)
	{
		T arr;
		for (size_t i = 0;i < arr.size();++i)
		{
			auto v = Read<T::value_type>(is);
			if (std::holds_alternative<Error>(v))
				return Error{};
			arr[i] = std::move(std::get<T::value_type>(v));
		}
		return arr;
	}
}