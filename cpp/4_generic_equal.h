#pragma once

#include <meta>
#include <type_traits>

// ============================ Demo 4 — Generic equality comparison ============================
// Before C++26: you'd write operator== comparing each field, or use
// C++20's defaulted operator==. Reflection gives you a third option
// that works on any struct without modifying it.

/**
 * @brief Compare two structs for equality by iterating all fields via reflection.
 *
 * Works on any aggregate without requiring operator== to be defined.
 *
 * @tparam T An aggregate type (enforced by the requires clause).
 * @param a First object to compare.
 * @param b Second object to compare.
 * @return true if all fields are equal, false otherwise.
 */
template <typename T>
    requires std::is_aggregate_v<T>
bool generic_equal (const T& a, const T& b)
{
    bool equal = true;

    // Same pattern: reflect the type, iterate its fields at compile time,
    // and splice each field into a comparison expression.
    template for (constexpr auto member :
                  define_static_array (nonstatic_data_members_of (^^T,
                                                                  std::meta::access_context::unchecked())))
    {
        // a.[:member:] and b.[:member:] splice the same field reflection
        //   into member access on two different objects.
        if (a.[:member:] != b.[:member:])
            equal = false;
    }

    return equal;
}
