#include "ZSerializer.h"
#include <cstdint>
#include <sstream>
#include <tuple>


template<typename T>
void Check(std::istream& is, T target)
{
    auto got = zs::Read<T>(is);
    if (std::holds_alternative<zs::Error>(got))
        abort();
    if (std::get<T>(got) != target)
        abort();
}

struct POD
{
    char c;
    int i;

    bool operator ==(const POD&) const = default;
};

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

        POD{ 'a',33 },

        // "the",
        // "quick"sv,
        std::string("brown"),

        std::optional<std::string>{"fox"},
        std::optional<POD>{std::nullopt},

        std::vector<int32_t>{1, 2, 3},
        std::vector<std::string>{"jumps", "over", "the"},

        std::array<float, 3>{10.f, 12.f, 33.f},

        std::array<std::string, 2>{"lazy", "dog"}
    );

    std::ostringstream oss;
    std::apply([&oss](auto&&... args) {(zs::Write(oss, args), ...); }, data);

    std::istringstream iss(oss.str());
    std::apply([&iss](auto&&... args) {(Check(iss, args), ...); }, data);

    return 0;
}