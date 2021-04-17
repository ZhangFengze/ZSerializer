#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "../ZSerializer.h"
#include <cstdint>
#include <sstream>
#include <tuple>

using namespace std::literals;

struct Vec3
{
    float x, y, z;
    bool operator ==(const Vec3&) const = default;
};

struct State
{
    std::string name;
    float hp;
    Vec3 pos;
    Vec3 vel;
    bool operator ==(const State&) const = default;
};

template<typename T>
void Check(std::istream& is, T target)
{
    auto result = zs::Read<T>(is);
    REQUIRE(std::holds_alternative<T>(result));
    REQUIRE(std::get<T>(result) == target);
}

TEST_CASE("basic types")
{
    zs::StringWriter out;

    zs::Write(out, true);
    zs::Write(out, false);

    zs::Write(out, int8_t(39));
    zs::Write(out, int16_t(30045));
    zs::Write(out, int32_t(993));
    zs::Write(out, int64_t(9931234));

    zs::Write(out, uint8_t(240));
    zs::Write(out, uint16_t(20033));
    zs::Write(out, uint32_t(330));
    zs::Write(out, uint64_t(783));

    zs::Write(out, 103.f);
    zs::Write(out, 48901.0);

    zs::Write(out, nullptr);
    zs::Write(out, (void*)(0x12345678));

    std::istringstream iss(out.Str());

    Check(iss, true);
    Check(iss, false);

    Check(iss, int8_t(39));
    Check(iss, int16_t(30045));
    Check(iss, int32_t(993));
    Check(iss, int64_t(9931234));

    Check(iss, uint8_t(240));
    Check(iss, uint16_t(20033));
    Check(iss, uint32_t(330));
    Check(iss, uint64_t(783));

    Check(iss, 103.f);
    Check(iss, 48901.0);

    Check(iss, nullptr);
    Check(iss, (void*)(0x12345678));
}

TEST_CASE("string")
{
    zs::StringWriter out;

    zs::Write(out, "the");
    zs::Write(out, "quick"sv);
    zs::Write(out, std::string("brown"));

    std::istringstream iss(out.Str());

    Check(iss, std::string("the"));
    Check(iss, std::string("quick"));
    Check(iss, std::string("brown"));
}

namespace zs
{
    template<>
    struct Trait<State> : public WriteMembers<State>, public ReadMembers<State>
    {
        static constexpr auto members = std::make_tuple
        (
            &State::name,
            &State::hp,
            &State::pos,
            &State::vel
        );
    };
}

TEST_CASE("custom")
{
    zs::StringWriter out;

    zs::Write(out, State{ "tom", 99.f, {3.f,10.f,99.f}, {1.4f,0.f,3.f} });

    std::istringstream iss(out.Str());

    Check(iss, State{ "tom", 99.f, {3.f,10.f,99.f}, {1.4f,0.f,3.f} });
}

TEST_CASE("optional")
{
    zs::StringWriter out;

    zs::Write(out, std::optional<std::string>{"fox"});
    zs::Write(out, std::optional<Vec3>{std::nullopt});

    std::istringstream iss(out.Str());

    Check(iss, std::optional<std::string>("fox"));
    Check(iss, std::optional<Vec3>{std::nullopt});
}

TEST_CASE("vector")
{
    zs::StringWriter out;

    zs::Write(out, std::vector<int32_t>{1, 2, 3});
    zs::Write(out, std::vector<std::string>{"jumps", "over", "the"});

    std::istringstream iss(out.Str());

    Check(iss, std::vector<int32_t>{1, 2, 3});
    Check(iss, std::vector<std::string>{"jumps", "over", "the"});
}

TEST_CASE("array")
{
    zs::StringWriter out;

    zs::Write(out, std::array<float, 3>{10.f, 12.f, 33.f});
    zs::Write(out, std::array<std::string, 2>{"lazy", "dog"});
    zs::Write(out, std::array<State, 16>{State{ "Jerry", 12.f,{0,0,0},{0,0,1} }});

    std::istringstream iss(out.Str());

    Check(iss, std::array<float, 3>{10.f, 12.f, 33.f});
    Check(iss, std::array<std::string, 2>{"lazy", "dog"});
    Check(iss, std::array<State, 16>{State{ "Jerry", 12.f,{0,0,0},{0,0,1} }});
}