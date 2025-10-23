# Build Instructions
This plugin is built with the Cheat Engine plugin SDK and Lua 5.3 libs included with Cheat Engine. To build, you need to link the `lua53-64/32.lib` and update the paths to `lua.hpp` and `cepluginsdk.h`, all located in `<Cheat Engine Install Directory>\plugins`, usually at `C:\Program Files\Cheat Engine\plugins` if you used the default install location

# Installation instructions
After building or downloading the .dll, open Cheat Engine, go to Edit > Settings > Plugins, hit Add New, and add the dll and enable the plugin. To use the plugin, just call the function bl4_decode(serial) in Lua, replacing serial with a valid BL4 serial. The function returns a string with the deserialized data.
