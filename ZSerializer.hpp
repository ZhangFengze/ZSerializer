#pragma once
#include <concepts>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <optional>
#include <vector>
#include <array>

#define ZS_READ(type, in, name)\
		type name;\
		if(auto temp = Read<type>(in); std::holds_alternative<Error>(temp))\
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

	struct StringWriter
	{
		void Write(const void* source, size_t bytes)
		{
			os.write(reinterpret_cast<const char*>(source), bytes);
		}

		std::string String() const
		{
			return os.str();
		}

		std::ostringstream os;
	};

	template<typename T, typename Out>
	void Write(Out& out, const T& value);

	template<typename T>
	struct WriteMembers
	{
		template<typename Out>
		static void Write(Out& out, const T& value)
		{
			using zs::Write;
			std::apply([&out, &value](auto&&... args){(Write(out, value.*args), ...);}, Trait<T>::members);
		}
	};
	
	template<typename T>
	struct WriteBitwise
	{
		template<typename Out>
		static void Write(Out& out, const T& value)
		{
			out.Write(std::addressof(value), sizeof(value));
		}
	};

	template<DefinedWriteTrait T, typename Out>
	void Write(Out& out, const T& value)
	{
		Trait<T>::Write(out, value);
	}

	template<POD T, typename Out>
	void Write(Out& out, const T& value)
	{
		out.Write(std::addressof(value), sizeof(value));
	}

	template<Optional T, typename Out>
	void Write(Out& out, const T& value)
	{
		if (value)
		{
			Write(out, true);
			Write(out, *value);
		}
		else
		{
			Write(out, false);
		}
	}

	template<typename Out>
	void Write(Out& out, const char* value)
	{
		size_t size = strlen(value);
		Write(out, size);
		out.Write(value, size);
	}

	template<typename T, typename Out>
		requires (Vector<T>&& POD<typename T::value_type>) || String<T> || StringView<T>
	void Write(Out& out, const T& value)
	{
		Write(out, value.size());
		out.Write(value.data(), value.size() * sizeof(T::value_type));
	}

	template<typename T, typename Out> requires !POD<T>
	void Write(Out& out, const std::vector<T>& vec)
	{
		Write(out, vec.size());
		for (const auto& v : vec)
			Write(out, v);
	}

	template<typename T, size_t size, typename Out> requires !POD<T>
	void Write(Out& out, const std::array<T, size>& arr)
	{
		for (const auto& v : arr)
			Write(out, v);
	}

	struct StringReader
	{
		StringReader(const std::string& str):is(str){}

		bool Read(void* dest, size_t bytes)
		{
			is.read(reinterpret_cast<char*>(dest), bytes);
			return is.gcount() == bytes;
		}

		std::istringstream is;
	};

	struct Error {};

	template<typename T, typename In>
	std::variant<T, Error> Read(In& in);

	template<typename T>
	struct ReadMembers
	{
		template<typename In>
		struct TryRead
		{
			TryRead(In& in, T& value, bool& failed)
				:in_(in), v_(value), failed_(failed){}

			bool& failed_;
			In& in_;
			T& v_;

			template<typename PointerToMember>
			void operator()(PointerToMember member) const
			{
				if(failed_)
					return;
				using Member = std::decay_t<decltype(std::declval<T>().*member)>;
				using zs::Read;
				auto temp=Read<Member>(in_);
				if(std::holds_alternative<Error>(temp))
				{
					failed_=true;
					return;
				}
				v_.*member=std::get<Member>(temp);
			}
		};

		template<typename In>
		static std::variant<T, Error> Read(In& in)
		{
			T value;
			bool failed = false;
			ForEach(Trait<T>::members, TryRead(in, value, failed));
			if(failed)
				return Error{};
			return value;
		}
	};

	template<typename T>
	struct ReadBitwise
	{
		template<typename In>
		static std::variant<T, Error> Read(In& in)
		{
			T value;
			if (!in.Read(std::addressof(value), sizeof(value)))
				return Error{};
			return value;
		}
	};

	template<DefinedReadTrait T, typename In>
	std::variant<T, Error> Read(In& in)
	{
		return Trait<T>::Read(in);
	}

	template<POD T, typename In>
	std::variant<T, Error> Read(In& in)
	{
		T value;
		if (!in.Read(std::addressof(value), sizeof(value)))
			return Error{};
		return value;
	}

	template<Optional T, typename In>
	std::variant<T, Error> Read(In& in)
	{
		ZS_READ(bool, in, hasValue);
		if (!hasValue)
			return std::nullopt;
		ZS_READ(typename T::value_type, in, value);
		return T(value);
	}

	template<typename T, typename In>
		requires (Vector<T>&& POD<typename T::value_type>) || String<T>
	std::variant<T, Error> Read(In& in)
	{
		ZS_READ(size_t, in, size);

		T value;
		value.resize(size);
		if (!in.Read(value.data(), value.size() * sizeof(T::value_type)))
			return Error{};
		return value;
	}

	template<typename T, typename In> requires Vector<T> && !POD<typename T::value_type>
	std::variant<T, Error> Read(In& in)
	{
		ZS_READ(size_t, in, size);

		T vec;
		for (size_t i = 0;i < size;++i)
		{
			ZS_READ(typename T::value_type, in, v);
			vec.emplace_back(std::move(v));
		}
		return vec;
	}

	template<typename T, typename In> requires Array<T> && !POD<typename T::value_type>
	std::variant<T, Error> Read(In& in)
	{
		T arr;
		for (size_t i = 0;i < arr.size();++i)
		{
			ZS_READ(typename T::value_type, in, v);
			arr[i] = std::move(v);
		}
		return arr;
	}
}