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

#define ZS_READ(type, is, name)\
		type name;\
		if(auto temp = Read<type>(is); std::holds_alternative<Error>(temp))\
			return Error{};\
		else\
			name = std::get<type>(temp);

namespace zs
{
	template<typename Tuple, typename Func>
	void ForEach(Tuple&& tuple, Func&& func)
	{
		std::apply([&func](auto&& ...args){(func(args),...);}, tuple);
	}

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

	template<typename T>
	struct Trait;

	template<typename T>
	concept DefinedWriteTrait = requires{ Trait<T>::Write; };

	template<typename T>
	concept DefinedReadTrait = requires{ Trait<T>::Read; };

	void Write(std::ostream& os, const void* source, size_t bytes)
	{
		os.write(reinterpret_cast<const char*>(source), bytes);
	}

	template<typename T>
	void Write(std::ostream& os, const T& value);

	template<typename T>
	struct WriteMembers
	{
		static void Write(std::ostream& os, const T& value)
		{
			using zs::Write;
			std::apply([&os, &value](auto&&... args){(Write(os, value.*args), ...);}, Trait<T>::members);
		}
	};

	template<DefinedWriteTrait T>
	void Write(std::ostream& os, const T& value)
	{
		Trait<T>::Write(os, value);
	}

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

	void Write(std::ostream& os, const char* value)
	{
		size_t size = strlen(value);
		Write(os, size);
		Write(os, value, size);
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

	template<typename T>
	struct ReadMembers
	{
		struct TryRead
		{
			TryRead(std::istream& is, T& value, bool& failed)
				:is_(is), v_(value), failed_(failed){}

			bool& failed_;
			std::istream& is_;
			T& v_;

			template<typename PointerToMember>
			void operator()(PointerToMember member) const
			{
				if(failed_)
					return;
				using Member = std::decay_t<decltype(std::declval<T>().*member)>;
				using zs::Read;
				auto temp=Read<Member>(is_);
				if(std::holds_alternative<Error>(temp))
				{
					failed_=true;
					return;
				}
				v_.*member=std::get<Member>(temp);
			}
		};

		static std::variant<T, Error> Read(std::istream& is)
		{
			T value;
			bool failed = false;
			ForEach(Trait<T>::members, TryRead(is, value, failed));
			if(failed)
				return Error{};
			return value;
		}
	};

	template<DefinedReadTrait T>
	std::variant<T, Error> Read(std::istream& is)
	{
		return Trait<T>::Read(is);
	}

	template<POD T>
	std::variant<T, Error> Read(std::istream& is)
	{
		T value;
		if (!Read(is, std::addressof(value), sizeof(value)))
			return Error{};
		return value;
	}

	template<Optional T>
	std::variant<T, Error> Read(std::istream& is)
	{
		ZS_READ(bool, is, hasValue);
		if (!hasValue)
			return std::nullopt;
		ZS_READ(typename T::value_type, is, value);
		return T(value);
	}

	template<typename T>
		requires (Vector<T>&& POD<typename T::value_type>) || String<T>
	std::variant<T, Error> Read(std::istream& is)
	{
		ZS_READ(size_t, is, size);

		T value;
		value.resize(size);
		if (!Read(is, value.data(), value.size() * sizeof(T::value_type)))
			return Error{};
		return value;
	}

	template<typename T> requires Vector<T> && !POD<typename T::value_type>
	std::variant<T, Error> Read(std::istream& is)
	{
		ZS_READ(size_t, is, size);

		T vec;
		for (size_t i = 0;i < size;++i)
		{
			ZS_READ(typename T::value_type, is, v);
			vec.emplace_back(std::move(v));
		}
		return vec;
	}

	template<typename T> requires Array<T> && !POD<typename T::value_type>
	std::variant<T, Error> Read(std::istream& is)
	{
		T arr;
		for (size_t i = 0;i < arr.size();++i)
		{
			ZS_READ(typename T::value_type, is, v);
			arr[i] = std::move(v);
		}
		return arr;
	}
}