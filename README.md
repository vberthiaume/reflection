# C++26 Reflection Demo

Two projects demonstrating reflection — one in C++26 (using P2996) and one in Python — to illustrate the same concepts side by side.

Both projects demonstrate:
1. **Enum <-> string conversion** — no hand-written switch statements
2. **Automatic JSON serialization** — serialize any struct without boilerplate
3. **Generic "describe"** — inspect and print any object's fields and types
4. **Generic equality comparison** — compare structs field-by-field without writing operator==

## C++ (P2996)

C++26 reflection requires a compiler that supports P2996. Currently the best option on macOS is [Bloomberg's clang-p2996 fork](https://github.com/bloomberg/clang-p2996).

### Building the compiler

```bash
# Install prerequisites
brew install cmake ninja

# Clone and build the p2996 fork
git clone https://github.com/bloomberg/clang-p2996.git
cd clang-p2996
git checkout p2996
cmake -S llvm -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_PROJECTS="clang" \
  -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind"
ninja -C build
```

This takes ~30-60 minutes. The compiler will be at `<clone-dir>/build/bin/clang++`.

### Building the demo

```bash
cd cpp
cmake -B build -G Ninja \
  -DCMAKE_CXX_COMPILER=<path-to-clang-p2996>/build/bin/clang++
ninja -C build
./build/reflection_demo
```

### Note on IDE errors

Your IDE's built-in clang will flag errors on `^^`, `[: :]`, and `template for` — these are expected. The code compiles correctly with the p2996 fork.

## Python

The Python demo uses only the standard library (no dependencies, no venv needed).

```bash
cd python
python3 reflection_demo.py
```

## Key takeaway

Python gets reflection for free at runtime via `__dict__`, `getattr`, `type()`, etc. C++26 gives you the same power but at **compile time with zero runtime cost** — the compiler resolves `^^T` and `[: :]` and emits ordinary code.

| Concept | C++26 | Python |
|---|---|---|
| Reflect a type | `^^T` | `type(obj)` |
| Get field names | `identifier_of(member)` | `vars(obj).keys()` |
| Get type name | `display_string_of(^^T)` | `type(obj).__name__` |
| Iterate fields | `template for` over `nonstatic_data_members_of()` | `for k, v in vars(obj).items()` |
| Iterate enum values | `template for` over `enumerators_of()` | `Enum.__members__` |
| Access field by reflection | `obj.[:member:]` | `getattr(obj, name)` |
