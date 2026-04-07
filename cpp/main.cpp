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
 *
 * We demonstrate four practical use cases:
 *   1. Enum ↔ string conversion (no hand-written switch statements!)
 *   2. Automatic struct-to-JSON serialization
 *   3. A generic "describe" function that prints any struct's fields
 *   4. Compile-time struct field iteration for generic equality comparison
 */

#include <meta>
#include <iostream>
#include <string>
#include <sstream>
#include <type_traits>
#include <expected>

struct Vector3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

enum class Color
{
    Red,
    Green,
    Blue,
    Yellow,
    Magenta,
    Cyan
};

struct Player
{
    std::string name;
    int         health   = 100;
    Vector3     position = {};
    Color       color    = Color::Red;
};

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

// =========================================================================
// Demo 2 — Automatic JSON serialization
// Before C++26: you'd hand-write a to_json() method for every struct, listing each field manually. Adding a field meant updating the method.
// With reflection: one generic function serializes ANY aggregate.

// Forward-declaring to_json() so we can use it right below when recursing
template <typename T>
std::string to_json (const T& obj);

/**
 * @brief Serialize a single value to a JSON fragment, dispatching on type.
 *
 * Handles strings, enums, arithmetic types, and nested aggregates.
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
 *
 * Uses reflection to iterate all fields — no manual field listing needed.
 *
 * @tparam T An aggregate (struct) type.
 * @param obj The object to serialize.
 * @return A JSON-formatted string (e.g., { "name": "Alice", "health": 95 }).
 */
template <typename T>
std::string to_json (const T& obj)
{
    std::ostringstream os;
    os << "{ ";

    bool first = true;

    // ^^T reflects the struct type T.
    // nonstatic_data_members_of() returns a vector of std::meta::info —
    //   one per field (e.g., name, health, position, color).
    // std::meta::access_context::unchecked() bypasses access checking (lets us
    //   reflect private members too if needed).
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

// =========================================================================
// Demo 4 — Generic equality comparison
// =========================================================================
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

// =========================================================================
// main — run all demos
// =========================================================================

int main()
{
    Player player { "Alice", 95, { 10.0f, 20.0f, 30.0f }, Color::Blue };

    // --- Demo 1: enum ↔ string ---
    std::cout << "============================================================\n";
    std::cout << "DEMO 1 — Enum <-> string conversion (no switch statement!)\n";
    std::cout << "============================================================\n";
    std::cout << "Color::Blue   -> \"" << enum_to_string (Color::Blue) << "\"\n";
    std::cout << "Color::Yellow -> \"" << enum_to_string (Color::Yellow) << "\"\n";

    auto parsed = string_to_enum<Color> ("Magenta");
    if (parsed)
        std::cout << "\"Magenta\"     -> Color::" << enum_to_string (*parsed) << "\n";
    else
        std::cout << parsed.error() << "\n";

    std::cout << "\n";

    // --- Demo 2: automatic JSON serialization ---
    std::cout << "============================================================\n";
    std::cout << "DEMO 2 — Automatic JSON serialization\n";
    std::cout << "============================================================\n";
    std::cout << to_json (player) << "\n\n";

    // --- Demo 3: generic describe ---
    std::cout << "============================================================\n";
    std::cout << "DEMO 3 — Generic struct inspection (describe)\n";
    std::cout << "============================================================\n";
    describe (player);
    std::cout << "\n";

    // --- Demo 4: generic equality ---
    std::cout << "============================================================\n";
    std::cout << "DEMO 4 — Generic field-by-field equality\n";
    std::cout << "============================================================\n";
    Vector3 a { 1.0f, 2.0f, 3.0f };
    Vector3 b { 1.0f, 2.0f, 3.0f };
    Vector3 c { 1.0f, 2.0f, 9.0f };

    std::cout << "a = " << to_json (a) << "\n";
    std::cout << "b = " << to_json (b) << "\n";
    std::cout << "c = " << to_json (c) << "\n";
    std::cout << "generic_equal(a, b) = " << std::boolalpha << generic_equal (a, b) << "\n";
    std::cout << "generic_equal(a, c) = " << std::boolalpha << generic_equal (a, c) << "\n";

    return 0;
}
