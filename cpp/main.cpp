/* C++26 (P2996, https://isocpp.org/files/papers/P2996R13.html) introduces compile-time reflection
* with the following operators:
 *   ^^T                    — the "reflect" operator: produces a std::meta::info value
 *                            describing T (which can be type, namespace, function, value, or templates)
 *   [: r :]                — the "splice" operator: turns a std::meta::info back into
 *                            a language construct (type, expression, etc.)
 *   template for           — compile-time expansion loop: iterates over a range of
 *                            reflections, unrolled by the compiler into one block per element.
 *   define_static_array()  — converts a vector of std::meta::info into a static
 *                            array that template for can iterate over.
 *
 * All reflection happens at compile time (consteval). Some of the functions, e.g., enum_to_string()
 * are constexpr but this isn't a reflection requirement.
 */

#include "types.h"
#include "1_enum_string.h"
#include "2_json_serialize.h"
#include "3_describe.h"
#include "4_generic_equal.h"

#include <iostream>

int main()
{
    Player player { "Alice", 95, { 10.0f, 20.0f, 30.0f }, Color::Blue };

    std::cout << "============ DEMO 1 — Enum <-> string conversion (no switch statement!) ============\n";
    std::cout << "Color::Blue   -> \"" << mirror::enum_to_string (Color::Blue) << "\"\n";
    std::cout << "Color::Yellow -> \"" << mirror::enum_to_string (Color::Yellow) << "\"\n";
    if (const auto parsed { mirror::string_to_enum<Color> ("Magenta")})
        std::cout << "\"Magenta\"     -> Color::" << mirror::enum_to_string (*parsed) << "\n";
    else
        std::cout << parsed.error() << "\n";


    std::cout << "\n\n============ DEMO 2 — Automatic JSON serialization ============\n";
    std::cout << mirror::to_json (player) << "\n";


    std::cout << "\n\n============ DEMO 3 — Generic struct inspection (describe) ============\n";
    mirror::describe (player);


    std::cout << "\n\n============ DEMO 4 — Generic field-by-field equality ============\n";
    Vector3 a { 1.0f, 2.0f, 3.0f };
    Vector3 b { 1.0f, 2.0f, 3.0f };
    Vector3 c { 1.0f, 2.0f, 9.0f };

    std::cout << "a = " << mirror::to_json (a) << "\n";
    std::cout << "b = " << mirror::to_json (b) << "\n";
    std::cout << "c = " << mirror::to_json (c) << "\n";
    std::cout << "mirror::generic_equal(a, b) = " << std::boolalpha << mirror::generic_equal (a, b) << "\n";
    std::cout << "mirror::generic_equal(a, c) = " << std::boolalpha << mirror::generic_equal (a, c) << "\n\n";

    return 0;
}
