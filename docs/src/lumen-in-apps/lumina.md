# Lumina — [Visual Game Creation](https://github.com/spikest3r/lumencreator)

[Lumina](https://github.com/spikest3r/lumencreator) is a visual game creation environment that uses Lumen as its core scripting language. It allows developers to build 3D games and interactive projects using either visual block-based programming or by writing LumenLang code directly, providing flexibility in how game logic is expressed.

## Dual creation approaches

Lumina supports two complementary ways to build game logic:

- **Visual blocks:** A drag-and-drop block editor for movement, logic, physics, variables, and interactions — no coding required.
- **Lumen code:** Write game logic directly in LumenLang, accessing the same engine functions as the block editor.

Developers can switch between the two approaches within the same project, using blocks for high-level flow and Lumen code for complex algorithms or fine-grained control.

## The Lumina editor

Lumina's interface includes:

- **Viewport:** A 3D grid-based level editor with voxel-like placement and full positioning/rotation control
- **Toolbar:** Quick access to brushes for placing static tiles (walls, platforms, decorations)
- **Object menu:** Add interactive objects with physics support
- **Block editor or code editor:** Switch between visual programming and LumenLang source (toggle with F12)
- **Properties panel:** Adjust object properties and behaviors
- **File menu:** Save, load, and test projects in real-time (press F5 to play)

## Visual block programming

The block editor provides a full range of blocks for:

- **Movement:** Position and rotation control
- **Physics:** Gravity, collisions, and grounding checks
- **Control flow:** If/else conditionals, loops (repeat, while, forever)
- **Variables:** Local and global variable creation and manipulation
- **Logic:** Comparisons, boolean operations
- **Interaction:** Key press detection, collision detection, dialogue
- **Camera:** Camera positioning relative to the player
- **Procedures:** Define and call reusable routines

Blocks are type-aware, so slot connections validate that compatible data types fit together.

## Scripting with Lumen

For complex game logic, developers write LumenLang directly. The language compiles to bytecode executed by Lumina's built-in virtual machine, providing access to engine functions for movement, physics, input, and more. All standard Lumen features apply: variables, functions, loops, conditionals, and the standard library.

## Examples & templates

Lumina ships with ready-to-play example projects demonstrating common patterns:

- **parkour.lumina** — A platformer showcasing movement and jumping mechanics
- **collector.lumina** — A collection-based gameplay example
- **template_movement.lumina** — Starter template for basic movement controls
- **template_camera_movement.lumina** — Starter template for camera-relative controls

Load any example from the editor to explore how it works, then modify it as a starting point for your own projects.

## Technology & platforms

Lumina is built with:

- **Graphics:** Vulkan (via VulkanEngine) for high-performance 3D rendering
- **Window management:** GLFW3
- **UI:** ImGui for editor controls
- **Language:** C++20
- **Build system:** CMake

Lumina runs on **Linux and macOS** and requires the Vulkan SDK for building.

## Related projects

- **[LumenLang](https://github.com/spikest3r/LumenLang)** — The Lumen programming language that powers Lumina's scripting
- **[VulkanEngine](https://github.com/spikest3r/VulkanEngine)** — The rendering engine underlying Lumina's graphics
