#pragma once

#include "1_enum_string.h"
#include "2_json_serialize.h"

#include <meta>
#include <iostream>
#include <type_traits>

// =========================================================================
// Demo 3 — Generic "describe" (type + field introspection)
// =========================================================================
// Prints the type name, then each field's name, type, and value.

/**
 * @brief Print a human-readable description of any struct.
 *
 * Outputs the type name, then each field's name, type, and current value
 * to std::cout.
 *
 * @tparam T Any struct type.
 * @param obj The object to describe.
 */
template <typename T>
void describe (const T& obj)
{
    // ^^T reflects the type T.
    // display_string_of() returns a human-readable string for any
    //   reflection (e.g., "Player", "int", "Vector3").
    std::cout << "Object of type '" << display_string_of (^^T) << "':\n";

    template for (constexpr auto member :
                  define_static_array (nonstatic_data_members_of (^^T,
                                                                  std::meta::access_context::unchecked())))
    {
        // type_of(member) returns a reflection of the field's type,
        //   e.g., reflecting 'int' for the 'health' field.
        // display_string_of() then turns that into a printable name.
        std::cout << "  " << identifier_of (member)
                  << " (" << display_string_of (type_of (member)) << ")"
                  << " = ";

        // Print the value — dispatch on type for formatting.
        const auto& val = obj.[:member:];
        using FieldType = std::remove_cvref_t<decltype (val)>;

        if constexpr (std::is_enum_v<FieldType>)
            std::cout << enum_to_string (val);
        else if constexpr (std::is_aggregate_v<FieldType>)
            std::cout << to_json (val); // show nested structs as JSON
        else
            std::cout << val;

        std::cout << "\n";
    }
}
