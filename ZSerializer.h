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
	concept Optional= Optional_<T>;

	template<typename T>
	constexpr bool Vector_ = false;
	template<typename T>
	constexpr bool Vector_<std::vector<T>> = true;
	template<typename T>
	concept Vector= Vector_<T>;

	template<typename T>
	constexpr bool Array_ = false;
	template<typename T, size_t size>
	constexpr bool Array_<std::array<T, size>> = true;
	template<typename T>
	concept Array= Array_<T>;

	template<typename T, typename U>
	concept Same = std::is_same_v<T, U>;

	class OutputArchive
	{
	public:
		OutputArchive(std::ostream& os)
			:os_(os){}

		void Write(const void* source, size_t bytes)
		{
			os_.write(reinterpret_cast<const char*>(source), bytes);
		}

		// TODO raw string literals is different to std::string?
		template<POD T>
		void Write(const T& value)
		{
			Write(std::addressof(value), sizeof(value));
		}

		template<typename T>
		void Write(const std::optional<T>& value)
		{
			if (value)
			{
				Write(true);
				Write(*value);
			}
			else
			{
				Write(false);
			}
		}

		template<typename T>
			requires (Vector<T> && POD<typename T::value_type>) || String<T> || StringView<T>
		void Write(const T& value)
		{
			Write(value.size());
			Write(value.data(), value.size() * sizeof(typename T::value_type));
		}

		template<typename T>
		void Write(const std::vector<T>& vec)
		{
			Write(vec.size());
			for (const auto& v : vec)
				Write(v);
		}

		template<typename T, size_t size> requires !POD<T>
		void Write(const std::array<T, size>& arr)
		{
			for (const auto& v : arr)
				Write(v);
		}

	private:
		std::ostream& os_;
	};

	class InputArchive
	{
	public:
		InputArchive(std::istream& is)
			:is_(is){}

		bool Read(void* dest, size_t bytes)
		{
			is_.read(reinterpret_cast<char*>(dest), bytes);
			return is_.gcount()==bytes;
		}

		struct Error{};

		template<POD T>
		std::variant<T, Error> Read()
		{
			T value;
			if(Read(std::addressof(value), sizeof(value)))
				return value;
			else
				return Error{};
		}

		template<Optional T>
		std::variant<T, Error> Read()
		{
			auto hasValue=Read<bool>();
			if(std::holds_alternative<Error>(hasValue))
				return Error{};

			if(std::get<bool>(hasValue))
			{
				auto value=Read<T::value_type>();
				if(std::holds_alternative<Error>(value))
					return Error{};
				return T(std::get<T::value_type>(value));
			}
			else
			{
				return std::nullopt;
			}
		}

		template<typename T>
			requires (Vector<T> && POD<typename T::value_type>) || String<T>
		std::variant<T, Error> Read()
		{
			auto size=Read<size_t>();
			if(std::holds_alternative<Error>(size))
				return Error{};

			T value;
			value.resize(std::get<size_t>(size));
			if(Read(value.data(), value.size()*sizeof(T::value_type)))
				return value;
			else
				return Error{};
		}

		template<typename T> requires Vector<T> && !POD<typename T::value_type>
		std::variant<T, Error> Read()
		{
			auto size=Read<size_t>();
			if(std::holds_alternative<Error>(size))
				return Error{};

			T vec;
			for(size_t i=0;i<std::get<size_t>(size);++i)
			{
				auto v=Read<typename T::value_type>();
				if(std::holds_alternative<Error>(v))
					return Error{};
				vec.emplace_back(std::move(std::get<typename T::value_type>(v)));
			}
			return vec;
		}

		template<typename T> requires Array<T> && !POD<typename T::value_type>
		std::variant<T, Error> Read()
		{
			T arr;
			for(size_t i=0;i<arr.size();++i)
			{
				auto v=Read<typename T::value_type>();
				if(std::holds_alternative<Error>(v))
					return Error{};
				arr[i]=std::move(std::get<typename T::value_type>(v));
			}
			return arr;
		}

	private:
		std::istream& is_;
	};
}
