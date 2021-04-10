#pragma once
#include <istream>
#include <ostream>
#include <memory>
#include <optional>
#include <variant>
#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include <concepts>

namespace zs
{
	template<typename T>
	concept IsPOD = std::is_pod_v<T>;

	template<typename T>
	constexpr bool IsOptional_ = false;
	template<typename T>
	constexpr bool IsOptional_<std::optional<T>> = true;
	template<typename T>
	concept IsOptional= IsOptional_<T>;

	template<typename T>
	constexpr bool IsVector_ = false;
	template<typename T>
	constexpr bool IsVector_<std::vector<T>> = true;
	template<typename T>
	concept IsVector= IsVector_<T>;

	template<typename T>
	constexpr bool IsArray_ = false;
	template<typename T, size_t size>
	constexpr bool IsArray_<std::array<T, size>> = true;
	template<typename T>
	concept IsArray= IsArray_<T>;

	template<typename T, typename U>
	concept IsSame = std::is_same_v<T, U>;

	class OutputArchive
	{
	public:
		OutputArchive(std::ostream& os)
			:os_(os){}

		// TODO raw string literals is different to std::string?
		template<IsPOD T>
		void Write(const T& value)
		{
			os_.write(reinterpret_cast<const char*>(std::addressof(value)), sizeof(value));
		}

		void Write(const std::string& value)
		{
			Write(value.size());
			os_.write(value.data(), value.size());
		}

		void Write(std::string_view value)
		{
			Write(value.size());
			os_.write(value.data(), value.size());
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

		template<IsPOD T>
		void Write(const std::vector<T>& vec)
		{
			Write(vec.size());
			os_.write(reinterpret_cast<const char*>(vec.data()), vec.size() * sizeof(T));
		}

		template<typename T>
		void Write(const std::vector<T>& vec)
		{
			Write(vec.size());
			for (const auto& v : vec)
				Write(v);
		}

		template<typename T, size_t size> requires !IsPOD<T>
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

		template<IsPOD T>
		std::variant<T, Error> Read()
		{
			T value;
			if(Read(std::addressof(value), sizeof(value)))
				return value;
			else
				return Error{};
		}

		template<typename T> requires IsSame<T, std::string>
		std::variant<std::string, Error> Read()
		{
			auto size=Read<size_t>();
			if(std::holds_alternative<Error>(size))
				return Error{};

			std::string value;
			value.resize(std::get<size_t>(size));
			if(Read(value.data(), value.size()))
				return value;
			else
				return Error{};
		}

		template<IsOptional T>
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
				return std::optional<T::value_type>(std::get<T::value_type>(value));
			}
			else
			{
				return std::nullopt;
			}
		}

		template<typename T> requires IsVector<T> && IsPOD<typename T::value_type>
		std::variant<T, Error> Read()
		{
			auto size=Read<size_t>();
			if(std::holds_alternative<Error>(size))
				return Error{};

			T vec;
			vec.resize(std::get<size_t>(size));
			if(Read(vec.data(), vec.size()*sizeof(T::value_type)))
				return vec;
			else
				return Error{};
		}

		template<typename T> requires IsVector<T> && !IsPOD<typename T::value_type>
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

		template<typename T> requires IsArray<T> && !IsPOD<typename T::value_type>
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
