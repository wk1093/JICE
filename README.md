# JICE (JSON into C++ Engine)

JICE is a minimal game engine that uses JSON files to define game objects,
projects, and scenes. It is designed to be simple to use and easy to extend.

## Structure
 - engine/jice/Engine: The core engine library that handles graphics, input, and
   game object management. Internally, it uses my graphics library, [EOGLL](https://github.com/wk1093/EOGLL)
 - engine/jice/jicc: The JICE Compiler, which takes a valid JICE project and 
   compiles the JSON files into C++ code
 - engine/jice/editor: The JICE Editor, a simple GUI editor for editing JICE
   projects and scenes (HIGHLY WIP)

## Todo
 - [ ] Editor project selection
 - [ ] Editor scene selection
 - [ ] Editor project creation
 - [ ] Engine scene change
 - [ ] Engine more attributes
 - [ ] Engine better input system
 - [ ] Engine 3D support (kinda part of more attributes)

## Building
JICE uses CMake to build. The compiler/editor requires CMake to be available on the command line.
Python is also required to be available on the command line to run the JICE Compiler.