#pragma once

#include "1_enum_string.h"
#include "2_json_serialize.h"

#include <meta>
#include <iostream>
#include <type_traits>

// ============================ Demo 3 — Generic "describe" type & field introspection ============================
namespace mirror {

/**
 * @brief Print a human-readable description of any struct. Outputs the type name,
 * then each field's name, type, and current value to std::cout.
 *
 * @tparam T Any struct type.
 * @param obj The object to describe.
 */
template <typename T>
void describe (const T& obj)
{
    // ^^T reflects the type T.
    // std::meta::display_string_of() returns a human-readable string for any reflection
    std::cout << "Object of type '" << std::meta::display_string_of (^^T) << "':\n";

    template for (constexpr auto member :
                  define_static_array (std::meta::nonstatic_data_members_of (^^T,
                                                                  std::meta::access_context::unchecked())))
    {
        // std::meta::type_of(member) returns a reflection of the field's type, e.g., reflecting 'int' for the 'health' field.
        // std::meta::display_string_of() then turns that into a printable name.
        std::cout << "  " << std::meta::identifier_of (member)
                  << " (" << std::meta::display_string_of (std::meta::type_of (member)) << ")"
                  << " = ";

        // Print the value — dispatch on type for formatting.
        const auto& val = obj.[:member:];
        using FieldType = std::remove_cvref_t<decltype (val)>;

        if constexpr (std::is_enum_v<FieldType>)
            std::cout << mirror::enum_to_string (val);
        else if constexpr (std::is_aggregate_v<FieldType>)
            std::cout << mirror::to_json (val); // show nested structs as JSON
        else
            std::cout << val;

        std::cout << "\n";
    }
}

} // namespace mirror
