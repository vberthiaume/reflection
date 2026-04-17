#pragma once

#include "1_enum_string.h"

#include <meta>
#include <string>
#include <sstream>
#include <type_traits>

// ============================ Demo 2 — Automatic JSON serialization ============================

// Forward-declaring to_json() so we can use it right below when recursing
template <typename T>
std::string to_json (const T& obj);

/**
 * @brief Serialize a single value to a JSON fragment, dispatching on type.
 * Handles strings, enums (thanks to enum_to_string from the first reflection example),
 * arithmetic types, and nested aggregates.
 *
 * @tparam T The type of the value (deduced).
 * @param val The value to serialize.
 * @return A JSON-formatted string fragment.
 */
template <typename T>
std::string value_to_json (const T& val)
{
    if constexpr (std::is_same_v<T, std::string>)
        return "\"" + val + "\"";
    else if constexpr (std::is_enum_v<T>)
        return "\"" + enum_to_string (val) + "\"";
    else if constexpr (std::is_arithmetic_v<T>)
        return std::to_string (val);
    else if constexpr (std::is_aggregate_v<T>)
        return to_json (val); // recurse into nested structs
    else
        return "\"<unsupported>\"";
}

/**
 * @brief Serialize any aggregate struct to a JSON string.
 * Uses reflection to iterate all fields — no manual field listing needed.
 *
 * @tparam T An aggregate (struct) type.
 * @param obj The object to serialize.
 * @return A JSON-formatted string (e.g., { "name": "Alice", "health": 95 }).
 */
template <typename T>
std::string to_json (const T& obj)
{
    std::ostringstream os("{ ", std::ios_base::ate);

    bool first = true;

    // ^^T reflects the struct type T.
    // nonstatic_data_members_of() returns a vector of std::meta::info — one per field (e.g., name, health, position, color).
    // std::meta::access_context::unchecked() bypasses access checking (lets us reflect private members too if needed).
    template for (constexpr auto member :
                  define_static_array (nonstatic_data_members_of (^^T,
                                                                  std::meta::access_context::unchecked())))
    {
        if (! first)
            os << ", ";
        first = false;

        // identifier_of(member) returns the field's source-code name
        //   as a string_view (e.g., "health").
        // obj.[:member:] splices the reflection back into a member access
        //   expression — equivalent to writing obj.health, obj.name, etc.
        os << "\"" << identifier_of (member) << "\": "
           << value_to_json (obj.[:member:]);
    }

    os << " }";
    return os.str();
}
