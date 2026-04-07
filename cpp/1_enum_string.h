#pragma once

/* C++26 (P2996, https://isocpp.org/files/papers/P2996R13.html) introduces compile-time reflection
 * with the following operators:
 *   ^^T                    — the "reflect" operator: produces a std::meta::info value
 *                            describing T (a type, enumerator, namespace, etc.)
 *   [: r :]                — the "splice" operator: turns a std::meta::info back into
 *                            a language construct (type, expression, etc.)
 *   template for           — compile-time expansion loop: iterates over a range of
 *                            reflections, unrolled by the compiler into one block per element.
 *   define_static_array()  — converts a vector of std::meta::info into a static
 *                            array that template for can iterate over.
 *
 * All reflection happens at compile time (consteval). There is no runtime
 * overhead — the compiler resolves everything and emits ordinary code.
 */

#include <meta>
#include <string>
#include <expected>
#include <type_traits>

// ============================ Demo 1 — Enum ↔ string conversion ================================

/**
 * @brief Convert any enum value to its name as a string.
 *
 * 'requires std::is_enum_v<E>' is a C++20 constraint that restricts this
 * template to only accept enum types. Without it, calling enum_to_string(42)
 * would compile but fail inside the body when ^^E tries to reflect a non-enum.
 *
 * 'constexpr' is NOT required by the reflection — ^^, [:], and "template for"
 * are all consteval (compile-time only) regardless. constexpr here just allows
 * the function itself to be evaluated at compile time if desired, e.g.:
 *   static_assert(enum_to_string(Color::Red) == "Red");
 * It would work fine as a regular (non-constexpr) function too.
 *
 * @tparam E An enum type (enforced by the requires clause).
 * @param value The enum value to convert.
 * @return The enumerator's source-code name (e.g., "Red"), or "<unknown>"
 *         if no enumerator matches.
 */
template <typename E>
    requires std::is_enum_v<E>
constexpr std::string enum_to_string (E value)
{
    std::string result = "<unknown>";

    /*  This basically iterates through all the declared enum values of E.
        - ^^E reflects the enum type E, producing a std::meta::info value.
        - from that, enumerators_of() returns a vector of std::meta::info, one per enumerator (Red, Green, ...)
        - define_static_array() converts the vector into a static array so it can be iterated with 'template for'.
            Not required by the P2996 standard, but needed by the Bloomberg clang-p2996 fork.
        - 'template for' unrolls the loop at compile time; the compiler generates one if-branch per enumerator.
    */
    template for (constexpr auto e : define_static_array (enumerators_of (^^E)))
    {
        // at this point, e is a std::meta::info, so [:e:] converts it into an actual enumerator value, e.g., Color::Red
        if ([:e:] == value)
            // identifier_of() returns the source-code name of the enumerator as a string_view (e.g., "Red").
            result = std::string (identifier_of (e));
    }

    return result;
}

/**
 * @brief Convert a string to an enum value.
 * @tparam E An enum type (enforced by the requires clause).
 * @param str The string to look up (e.g., "Magenta").
 * @return The matching enumerator, or an error string if not found. The error message uses identifier_of(^^E)
 *         to include the enum's type name via reflection.
 */
template <typename E>
    requires std::is_enum_v<E>
constexpr std::expected<E, std::string> string_to_enum (const std::string& str)
{
    // this is the exact same as above; compile-time-iterate through the possible enumerator values of E
    template for (constexpr auto e : define_static_array (enumerators_of (^^E)))
    {
        // but here we reverse the logic: identifier_of(e) gives us the enumerator's name as a string (e.g., "Magenta")
        if (identifier_of (e) == str)
            // and [:e:] splices it back into the actual enumerator value (e.g., Color::Magenta).
            return [:e:];
    }

    // Can't find the enumerator value. identifier_of(^^E) reflects the enum type to its source-code name (e.g., "Color").
    return std::unexpected ("\"" + str + "\" is not a valid enumerator value of " + std::string (identifier_of (^^E)));
}
