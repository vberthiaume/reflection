"""
Reflection Demo in Python
=========================
This mirrors the C++ demo to highlight the contrast. In Python, reflection
is built-in — there's no special syntax needed. In C++26, you get compile-
time reflection via ^^T and [:r:]. The key difference:

  C++:    compile-time, zero runtime cost, requires ^^/[::] syntax
  Python: runtime, dynamic, always available via __dict__, getattr, etc.

We demonstrate the same four use cases as the C++ version:
  1. Enum <-> string conversion
  2. Automatic struct-to-JSON serialization
  3. A generic "describe" function that prints any object's fields
  4. Generic equality comparison via field iteration
"""

import json
from enum import Enum


# =========================================================================
# Domain types — mirrors the C++ structs and enum
# =========================================================================

class Color(Enum):
    Red = 0
    Green = 1
    Blue = 2
    Yellow = 3
    Magenta = 4
    Cyan = 5


class Vector3:
    def __init__(self, x: float = 0.0, y: float = 0.0, z: float = 0.0):
        self.x = x
        self.y = y
        self.z = z


class Player:
    def __init__(self, name: str, health: int, position: Vector3, color: Color):
        self.name = name
        self.health = health
        self.position = position
        self.color = color


# =========================================================================
# Demo 1 — Enum <-> string conversion
# =========================================================================
# In C++26, you iterate enumerators_of(^^E) with reflection.
# In Python, Enum already has .name and __members__ — no work needed.

def enum_to_string(value: Enum) -> str:
    """Trivial in Python: every Enum already carries its name."""
    return value.name


def string_to_enum(enum_class: type, name: str):
    """Look up an enumerator by name. Returns None if not found."""
    return enum_class.__members__.get(name)


# =========================================================================
# Demo 2 — Automatic JSON serialization
# =========================================================================
# In C++26, you iterate nonstatic_data_members_of(^^T) to get field names
# and access values via obj.[:member:].
# In Python, vars(obj) or obj.__dict__ gives you the same thing for free.

def to_dict(obj) -> dict:
    """Recursively convert any object to a dict using __dict__."""
    result = {}
    for key, value in vars(obj).items():
        if isinstance(value, Enum):
            result[key] = value.name
        elif hasattr(value, "__dict__"):
            result[key] = to_dict(value)
        else:
            result[key] = value
    return result


def to_json(obj) -> str:
    return json.dumps(to_dict(obj))


# =========================================================================
# Demo 3 — Generic "describe"
# =========================================================================
# C++26 uses display_name_of(^^T) and type_of(member).
# Python uses type(obj).__name__ and type(value).__name__.

def describe(obj) -> None:
    print(f"Object of type '{type(obj).__name__}':")
    for field_name, value in vars(obj).items():
        if isinstance(value, Enum):
            display = value.name
        elif hasattr(value, "__dict__"):
            display = to_json(value)
        else:
            display = repr(value)
        print(f"  {field_name} ({type(value).__name__}) = {display}")


# =========================================================================
# Demo 4 — Generic equality comparison
# =========================================================================
# C++26 iterates nonstatic_data_members_of(^^T) and compares each field.
# Python iterates vars() and does the same.

def generic_equal(a, b) -> bool:
    """Compare two objects field-by-field using __dict__."""
    if type(a) is not type(b):
        return False
    return vars(a) == vars(b)


# =========================================================================
# main — run all demos, mirroring the C++ output
# =========================================================================

def main():
    player = Player("Alice", 95, Vector3(10.0, 20.0, 30.0), Color.Blue)

    # --- Demo 1: enum <-> string ---
    print("=" * 60)
    print("DEMO 1 — Enum <-> string conversion")
    print("=" * 60)
    print(f'Color.Blue   -> "{enum_to_string(Color.Blue)}"')
    print(f'Color.Yellow -> "{enum_to_string(Color.Yellow)}"')
    parsed = string_to_enum(Color, "Magenta")
    if parsed is not None:
        print(f'"Magenta"     -> Color.{enum_to_string(parsed)}')
    print()

    # --- Demo 2: automatic JSON serialization ---
    print("=" * 60)
    print("DEMO 2 — Automatic JSON serialization")
    print("=" * 60)
    print(to_json(player))
    print()

    # --- Demo 3: generic describe ---
    print("=" * 60)
    print("DEMO 3 — Generic object inspection (describe)")
    print("=" * 60)
    describe(player)
    print()

    # --- Demo 4: generic equality ---
    print("=" * 60)
    print("DEMO 4 — Generic field-by-field equality")
    print("=" * 60)
    a = Vector3(1.0, 2.0, 3.0)
    b = Vector3(1.0, 2.0, 3.0)
    c = Vector3(1.0, 2.0, 9.0)
    print(f"a = {to_json(a)}")
    print(f"b = {to_json(b)}")
    print(f"c = {to_json(c)}")
    print(f"generic_equal(a, b) = {generic_equal(a, b)}")
    print(f"generic_equal(a, c) = {generic_equal(a, c)}")


if __name__ == "__main__":
    main()
