/* C++26 (P2996) introduces compile-time reflection with the following operators:
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

struct Vector3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

enum class Color { Red, Green, Blue, Yellow, Magenta, Cyan };

struct Player
{
    std::string name;
    int         health   = 100;
    Vector3     position = {};
    Color       color    = Color::Red;
};

// =========================================================================
// Demo 1 — Enum ↔ string conversion
// Before C++26: you'd write a switch with one case per enumerator, or use
// a macro like X-macros. Adding a new enumerator meant updating the switch.
// With reflection: a single generic function handles ANY enum automatically.

/// Convert any enum value to its name as a string.
template <typename E>
    requires std::is_enum_v<E>
constexpr std::string enum_to_string(E value)
{
    std::string result = "<unknown>";

    // ^^E reflects the enum type. enumerators_of() returns a vector of
    // std::meta::info values — one per enumerator.
    // define_static_array() makes it usable in a template for loop.
    // For each enumerator, we splice it back into an expression with [: :]
    // and compare it to our runtime value.
    // ^^E reflects the enum type E, producing a std::meta::info value.
    // enumerators_of() takes that reflection and returns a vector of
    //   std::meta::info — one entry per enumerator (e.g., Red, Green, ...).
    // define_static_array() converts the vector into a static array so it
    //   can be iterated with 'template for'.
    // 'template for' unrolls the loop at compile time — the compiler
    //   generates one if-branch per enumerator with zero runtime overhead.
    template for (constexpr auto e : define_static_array(enumerators_of(^^E)))
    {
        // [:e:] is the splice operator — it turns the std::meta::info 'e'
        //   back into the actual enumerator value (e.g., Color::Red).
        if (value == [:e:])
            // identifier_of() returns the source-code name of the
            //   enumerator as a string_view (e.g., "Red").
            result = std::string(identifier_of(e));
    }

    return result;
}

/// Convert a string to an enum value (returns false if no match).
template <typename E>
    requires std::is_enum_v<E>
constexpr bool string_to_enum(const std::string& str, E& out)
{
    bool found = false;

    // Same pattern: reflect ^^E -> get enumerators -> iterate at compile time.
    template for (constexpr auto e : define_static_array(enumerators_of(^^E)))
    {
        // identifier_of(e) gives us the name; [:e:] gives us the value.
        if (str == identifier_of(e))
        {
            out = [:e:];
            found = true;
        }
    }

    return found;
}

// =========================================================================
// Demo 2 — Automatic JSON serialization
// =========================================================================
// Before C++26: you'd hand-write a to_json() method for every struct,
// listing each field manually. Adding a field meant updating the method.
// With reflection: one generic function serializes ANY aggregate.

// Forward-declare so nested structs work recursively.
template <typename T>
std::string to_json(const T& obj);

// Helper: serialize a single value depending on its type.
template <typename T>
std::string value_to_json(const T& val)
{
    if constexpr (std::is_same_v<T, std::string>)
        return "\"" + val + "\"";
    else if constexpr (std::is_enum_v<T>)
        return "\"" + enum_to_string(val) + "\"";
    else if constexpr (std::is_arithmetic_v<T>)
        return std::to_string(val);
    else if constexpr (std::is_aggregate_v<T>)
        return to_json(val);   // recurse into nested structs
    else
        return "\"<unsupported>\"";
}

/// Serialize any aggregate struct to a JSON string.
template <typename T>
std::string to_json(const T& obj)
{
    std::ostringstream os;
    os << "{ ";

    bool first = true;

    // nonstatic_data_members_of() gives us every field of the struct.
    // identifier_of(member) is the field name, and obj.[:member:] accesses
    // that field's value on our concrete object.
    // ^^T reflects the struct type T.
    // nonstatic_data_members_of() returns a vector of std::meta::info —
    //   one per field (e.g., name, health, position, color).
    // std::meta::access_context::unchecked() bypasses access checking (lets us
    //   reflect private members too if needed).
    template for (constexpr auto member :
                  define_static_array(nonstatic_data_members_of(^^T,
                      std::meta::access_context::unchecked())))
    {
        if (!first) os << ", ";
        first = false;

        // identifier_of(member) returns the field's source-code name
        //   as a string_view (e.g., "health").
        // obj.[:member:] splices the reflection back into a member access
        //   expression — equivalent to writing obj.health, obj.name, etc.
        os << "\"" << identifier_of(member) << "\": "
           << value_to_json(obj.[:member:]);
    }

    os << " }";
    return os.str();
}

// =========================================================================
// Demo 3 — Generic "describe" (type + field introspection)
// =========================================================================
// Prints the type name, then each field's name, type, and value.

template <typename T>
void describe(const T& obj)
{
    // ^^T reflects the type T.
    // display_string_of() returns a human-readable string for any
    //   reflection (e.g., "Player", "int", "Vector3").
    std::cout << "Object of type '" << display_string_of(^^T) << "':\n";

    template for (constexpr auto member :
                  define_static_array(nonstatic_data_members_of(^^T,
                      std::meta::access_context::unchecked())))
    {
        // type_of(member) returns a reflection of the field's type,
        //   e.g., reflecting 'int' for the 'health' field.
        // display_string_of() then turns that into a printable name.
        std::cout << "  " << identifier_of(member)
                  << " (" << display_string_of(type_of(member)) << ")"
                  << " = ";

        // Print the value — dispatch on type for formatting.
        const auto& val = obj.[:member:];
        using FieldType = std::remove_cvref_t<decltype(val)>;

        if constexpr (std::is_enum_v<FieldType>)
            std::cout << enum_to_string(val);
        else if constexpr (std::is_aggregate_v<FieldType>)
            std::cout << to_json(val);   // show nested structs as JSON
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

template <typename T>
    requires std::is_aggregate_v<T>
bool generic_equal(const T& a, const T& b)
{
    bool equal = true;

    // Same pattern: reflect the type, iterate its fields at compile time,
    // and splice each field into a comparison expression.
    template for (constexpr auto member :
                  define_static_array(nonstatic_data_members_of(^^T,
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
    std::cout << "Color::Blue   -> \"" << enum_to_string(Color::Blue) << "\"\n";
    std::cout << "Color::Yellow -> \"" << enum_to_string(Color::Yellow) << "\"\n";

    Color parsed;
    if (string_to_enum(std::string("Magenta"), parsed))
        std::cout << "\"Magenta\"     -> Color::" << enum_to_string(parsed) << "\n";

    std::cout << "\n";

    // --- Demo 2: automatic JSON serialization ---
    std::cout << "============================================================\n";
    std::cout << "DEMO 2 — Automatic JSON serialization\n";
    std::cout << "============================================================\n";
    std::cout << to_json(player) << "\n\n";

    // --- Demo 3: generic describe ---
    std::cout << "============================================================\n";
    std::cout << "DEMO 3 — Generic struct inspection (describe)\n";
    std::cout << "============================================================\n";
    describe(player);
    std::cout << "\n";

    // --- Demo 4: generic equality ---
    std::cout << "============================================================\n";
    std::cout << "DEMO 4 — Generic field-by-field equality\n";
    std::cout << "============================================================\n";
    Vector3 a { 1.0f, 2.0f, 3.0f };
    Vector3 b { 1.0f, 2.0f, 3.0f };
    Vector3 c { 1.0f, 2.0f, 9.0f };

    std::cout << "a = " << to_json(a) << "\n";
    std::cout << "b = " << to_json(b) << "\n";
    std::cout << "c = " << to_json(c) << "\n";
    std::cout << "generic_equal(a, b) = " << std::boolalpha << generic_equal(a, b) << "\n";
    std::cout << "generic_equal(a, c) = " << std::boolalpha << generic_equal(a, c) << "\n";

    return 0;
}
