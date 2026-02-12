# ImRefl - A C++26 reflection library for ImGui

A library utilising C++26 reflection features to generate ImGui rendering code at compile time for structs without the need for macro magic or addional boilerplate.

![Example Image](images/imrefl.png)

Simply include the header, declare your types and call `ImRefl::Input`:

```cpp
player main_player = {};
ImGui::Begin("Debug");
ImRefl::Input("Player", main_player);
ImGui::End();
```

That's it! No macros or other setup needed!

## Features
### Supported types
* Aggregate structs.
* Enum classes.
* All arithmetic types, including long double, and bool.
* `char` is treated as a character rather than an 8 bit integer. 
* `std::string`.
* `std::pair<L, R>`.
* `std::optional<T>`.
* `glm::vec2`, `glm::vec3` and `glm::vec4` from the GLM graphics library. 
    * These are not enabled by default. To enable, add the line `#define INREFL_GLM` above the include.

### Annotations

| Annotation                 | Description                         |
|----------------------------|-------------------------------------|
| `ImRefl::ignore`           | Skips the data member when rendering the widget |
| `ImRefl::readonly`         | Shows the field on the widget but makes it non-interactable |
| `ImRefl::slider(min, max)` | Changes the visual style from a simple text input into a slider with the given limits. |
| `ImRefl::color`            | Renders the annotated field as a color picker. Works for 3 and 4 dimensional arrays and spans as well as `glm::vec3` and `glm::vec4`. |
| `ImRegl::color_wheel`      | Similar to the above but a full color wheel. |

## Future work
* All reasonable standard library types (for some definition of reasonable).
* Support for third party types such as the GLM library.
* More annotations for other ImGui visual styles and customisation points.
