/*

This is the main unity file for a project.

*/


//~ Required Game level functions - must be declared here
// These are functions the engine needs to about related to the game.
// For example, once the engine is done setting up internal behaviors,
// it calls into "GameInit()"

void GameInit();
void GameUpdateAndRender();

//~ Required C++ Headers
//~ CLib  Headers
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

// NOTE(Dustin): Where is this actually used?
#include <stddef.h>
// Used for determining if a registered component
// inherits from IComponent
#include <type_traits>
// needed for placement new
#include <new>
// NOTE(Dustin): Maybe can replace with hashtable
#include <set>
// NOTE(Dustin): Maybe can replace with jstring
#include <string>
#include <new>
// NOTE(Dustin): I don't think this header is actually needed
#include <iostream>
// NOTE(Dustin): Handle this better: Used for VK_CHECK_RESULT
#include <stdexcept>
// NOTE(Dustin): Is this still necessary?
#include <optional>


//~ Dependencies




//~ Engine Source
// TODO(Dustin): Remove GLFW as a dependence

#include "engine/engine_unity.cpp"

//~ Game Source

#include "example/example_unity.cpp"
