# ImRefl - A C++26 reflection library for ImGui

A library utilising C++26 reflection features to generate ImGui rendering code at compile time for structs without the need for macros magic or addional boilerplate.

![Example Image](images/imrefl.png)

Simply declare your types and then expose them to ImGui:

```cpp
player main_player = {};
ImGui::Begin("Debug");
ImRefl::Input("Player", main_player);
ImGui::End();
```

That's it! No macros or other setup needed!

## Features
### Supported Types
* Aggregate structs.
* Enum classes.
* All arithmetic types, including long double, and bool.
* `char` is treated as a character rather than an 8 bit integer. 
* `std::string`.
* `std::pair<L, R>`.

### Helper Annotations

|--------------------------|-------------------------------------|
| Annotation               | Description                         |
|--------------------------|-------------------------------------|
| ImRefl::hidden           | Hides the annotated from the widget |
| ImRefl::readonly         | Shows the field on the widget but makes it non-interactable |
| ImRefl::slider(min, max) | Changes the visual style from a simple text input into a slider with the given limits. implemented only for arithmetic types |

## Future Work
* All reasonable standard library types (for some definition of reasonable).
* Support for third party types such as the glm library.
* More annotations for other ImGui visual styles and customisation points.
