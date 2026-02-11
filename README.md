# ImRefl - A C++26 reflection library for ImGui

A library utilising C++26 reflection features to generate ImGui rendering code at compile time for structs without the need for macros.

Simple declare your types:

```cpp
enum class weapon 
{
    none,
    sword,
    bow,
    staff,
    wand,
    mace,
    hammer,
    axe
};

struct player
{
    std::string name         = "Link";
    bool        invulnerable = false;
    int         health       = 100;

    [[=ImRefl::slider(1, 50)]]
    int level = 1;

    [[=ImRefl::hidden]]
    double secret_information = 3.14159;

    [[=ImRefl::readonly]]
    float attack_modifier = 3.5f;

    weapon current_weapon = weapon::sword;
};
```

Then expose them to ImGui:

```cpp
player main_player = {};
ImRefl::Input("Debug", main_player);
```

That's it!

## Supported Types
TBA

## Future Work
TBA
