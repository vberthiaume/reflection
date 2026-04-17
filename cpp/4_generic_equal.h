#pragma once

#include <meta>
#include <type_traits>

// ============================ Demo 4 — Generic equality comparison ============================
namespace mirror {

/**
 * @brief Compare two structs for equality by iterating all fields via reflection.
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
    // Same pattern: reflect the type, iterate its fields at compile time, and splice each field into a comparison expression.
    template for (constexpr auto member :
                  define_static_array (std::meta::nonstatic_data_members_of (^^T,
                                                                  std::meta::access_context::unchecked())))
    {
        // a.[:member:] and b.[:member:] splice the same field reflection into member access on two different objects.
        if (a.[:member:] != b.[:member:])
            return false;
    }

    return true;
}

} // namespace mirror
