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
TBA

## Future Work
TBA
