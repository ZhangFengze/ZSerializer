#include "ZSerializer.h"
#include <cstdint>
#include <sstream>
#include <tuple>

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

namespace zs
{
    template<>
    struct Trait<State>: public WriteMembers<State>, public ReadMembers<State>
    {
        static constexpr auto members=std::make_tuple
        (
            &State::name,
            &State::hp,
            &State::pos,
            &State::vel
        );
    };
}

template<typename T>
void Check(std::istream& is, T target)
{
    auto got = zs::Read<T>(is);
    if (std::holds_alternative<zs::Error>(got))
        abort();
    if (std::get<T>(got) != target)
        abort();
}

int main()
{
    using namespace std::literals;

    auto data = std::make_tuple
    (
        true,
        false,

        int8_t(39),
        int16_t(30045),
        int32_t(993),
        int64_t(9931234),

        uint8_t(240),
        uint16_t(20033),
        uint32_t(330),
        uint64_t(783),

        103.f,
        48901.0,

        State{"tom", 99.f, {3.f,10.f,99.f}, {1.4f,0.f,3.f} },

        // "the",
        // "quick"sv,
        std::string("brown"),

        std::optional<std::string>{"fox"},
        std::optional<Vec3>{std::nullopt},

        std::vector<int32_t>{1, 2, 3},
        std::vector<std::string>{"jumps", "over", "the"},

        std::array<float, 3>{10.f, 12.f, 33.f},

        std::array<std::string, 2>{"lazy", "dog"},

        std::array<State, 16>{State{ "Jerry", 12.f,{0,0,0},{0,0,1} }}
    );

    std::ostringstream oss;
    std::apply([&oss](auto&&... args) {(zs::Write(oss, args), ...); }, data);

    std::istringstream iss(oss.str());
    std::apply([&iss](auto&&... args) {(Check(iss, args), ...); }, data);

    return 0;
}