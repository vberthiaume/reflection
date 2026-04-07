#pragma once

#include <string>

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
