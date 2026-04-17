#pragma once

#include <meta>
#include <string>
#include <expected>
#include <type_traits>

// ============================ Demo 1 — Enum ↔ string conversion ============================
namespace mirror {

/**
 * @brief Convert any enum value to its name as a string.
 *
 * 'requires std::is_enum_v<T>' is a C++20 constraint that restricts this
 * template to only accept enum types. Without it, calling enum_to_string(42)
 * would compile but fail inside the body when ^^T tries to reflect a non-enum.
 *
 *
 * @tparam T An enum type (enforced by the requires clause).
 * @param value The enum value to convert.
 * @return The enumerator's source-code name (e.g., "Red"), or "<unknown>"
 *         if no enumerator matches.
 */
template <typename T>
    requires std::is_enum_v<T>
constexpr std::string enum_to_string (T value)
{
    std::string result = "<unknown>";

    /*  This basically iterates through all the declared enum values of T.
        - ^^T reflects the type into a std::meta::info value.
        - from that, std::meta::enumerators_of() returns a vector of std::meta::info, one per enumerator (Red, Green, ...)
        - define_static_array() converts the vector into a static array so it can be iterated with 'template for'.
            Not required by the P2996 standard, but needed by the Bloomberg clang-p2996 fork.
        - 'template for' unrolls the loop at compile time; the compiler generates one if-branch per enumerator.
    */
    template for (constexpr auto e : define_static_array (std::meta::enumerators_of (^^T)))
    {
        // at this point, e is a std::meta::info, so [:e:] converts it into an actual enumerator value, e.g., Color::Red
        if ([:e:] == value)
            // std::meta::identifier_of() returns the source-code name of the enumerator as a string_view (e.g., "Red").
            result = std::string (std::meta::identifier_of (e));
    }

    return result;
}

/**
 * @brief Convert a string to an enum value.
 * @tparam T An enum type (enforced by the requires clause).
 * @param str The string to look up (e.g., "Magenta").
 * @return The matching enumerator, or an error string if not found. The error message uses std::meta::identifier_of(^^T)
 *         to include the enum's type name via reflection.
 */
template <typename T>
    requires std::is_enum_v<T>
constexpr std::expected<T, std::string> string_to_enum (const std::string& str)
{
    // this is the exact same as above; compile-time-iterate through the possible enumerator values of T
    template for (constexpr auto e : define_static_array (std::meta::enumerators_of (^^T)))
    {
        // but here we reverse the logic: std::meta::identifier_of(e) gives us the enumerator's name as a string (e.g., "Magenta")
        if (std::meta::identifier_of (e) == str)
            // and [:e:] splices it back into the actual enumerator value (e.g., Color::Magenta).
            return [:e:];
    }

    // Can't find the enumerator value. std::meta::identifier_of(^^T) reflects the enum type to its source-code name (e.g., "Color").
    return std::unexpected ("\"" + str + "\" is not a valid enumerator value of " + std::string (std::meta::identifier_of (^^T)));
}

} // namespace mirror
