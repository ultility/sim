#pragma once

#define SDL_MAIN_HANDLED

#include "flecs.h"
#include <stdio.h>
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

void init();
int cleanup(SDL_Window* window, SDL_Renderer* renderer, ImGuiContext* ctx);