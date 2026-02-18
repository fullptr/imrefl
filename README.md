# ImRefl - A C++26 reflection library for ImGui

A library utilising C++26 reflection features to generate ImGui rendering code at compile time for structs without the need for macro magic or addional boilerplate.

This library is still very much of a proof-of-concept, and the interface will be unstable while I experiment with the ergonomics.

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
* All arithmetic types including `long double`.
* `bool` is rendered as a checkbox.
* `char` is treated as a character rather than an 8 bit integer. 
* `std::string`.
* `std::pair<L, R>`.
* `std::optional<T>`.
* `std::array<T, N>`, `std::span<T>` and C-style arrays `T[N]`.
* `std::variant<Ts...>` - however this can be improved; it currently uses typeid to get the type names, which can be hard to read.
* All vector types from the GLM graphics library, e.g. `glm::vec2` and `glm::ivec3`.
    * These are not enabled by default. To enable, add the line `#define INREFL_GLM` above the include.

### Annotations

| Annotation                 | Description                         |
|----------------------------|-------------------------------------|
| `ImRefl::ignore`           | Skips the data member when rendering the widget. |
| `ImRefl::readonly`         | Shows the field on the widget but makes it non-interactable. |
| `ImRefl::normal`           | Sets the visual style of arithmetic types (and arrays of) to a standard input box. This is the default option, but this is provided for consistency. |
| `ImRefl::slider(min, max)` | Changes the visual style of arithmetic types (and arrays of) to a slider with the given limits. |
| `ImRefl::drag(min, max, speed=1.0f)` | Changes the visual style of arithmetic types (and arrays of) to a dragger with the given limits and speed. |
| `ImRefl::color`            | Renders the annotated field as a color picker. Works for 3 and 4 dimensional arrays and spans as well as `glm::vec3` and `glm::vec4`. |
| `ImRefl::color_wheel`      | Similar to the above but a full color wheel. |
| `ImRefl::string` | For C-style char arrays, formats them as a fixed size string rather than a set of values. |
| `ImRefl::radio` | For enum classes. Displays the enum as a series of radio buttons rather than a dropdown. |

## Building the example
### Dependencies
- glfw3

From the top level of the repository, run:
```
cmake -S . -B build -DIMREFL_BUILD_EXAMPLE=ON -DCMAKE_CXX_COMPILER=/path/to/cxx26/compiler && cmake --build build
```
replacing `/path/to/cxx/compiler` with the path to your compiler. This will produce a binary at `build/example/imrefl-example`


## Importing the library
Using `ImRefl` in your own project is simple using CMake:
```cmake
# First include the source directory:
add_subdirectory(imrefl)

# OR with FetchContent:
include(FetchContent)
FetchContent_Declare(
    imrefl
    GIT_REPOSITORY https://github.com/fullptr/imrefl.git
)
FetchContent_MakeAvailable(imrefl)

# Then link the ImRefl interface
target_link_libraries(example PRIVATE ImRefl)
```

## Future work
* All reasonable standard library types (for some definition of reasonable).
* Support for third party types such as the GLM library.
* More annotations for other ImGui visual styles and customisation points.
