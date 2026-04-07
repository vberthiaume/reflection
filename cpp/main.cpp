#include "types.h"
#include "1_enum_string.h"
#include "2_json_serialize.h"
#include "3_describe.h"
#include "4_generic_equal.h"

#include <iostream>

int main()
{
    Player player { "Alice", 95, { 10.0f, 20.0f, 30.0f }, Color::Blue };

    // --- Demo 1: enum ↔ string ---
    std::cout << "============================================================\n";
    std::cout << "DEMO 1 — Enum <-> string conversion (no switch statement!)\n";
    std::cout << "============================================================\n";
    std::cout << "Color::Blue   -> \"" << enum_to_string (Color::Blue) << "\"\n";
    std::cout << "Color::Yellow -> \"" << enum_to_string (Color::Yellow) << "\"\n";

    const auto parsed = string_to_enum<Color> ("Magenta");
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
