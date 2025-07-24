#include "headers/main.hpp"

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int MAX_FPS = 60;
const int MAX_PHYSICS_FPS = 15;

struct Position {
    float x,y;
};

struct Velocity {
    float x,y;
};

struct Size {
    float w,h;
};

struct Drawable {
    uint8_t r,g,b,a;
};

int main(int argc, char** argv) {
    init();
    flecs::world world;
    Uint64 last_frame = 0, last_physics_frame = 0;
    flecs::query<Drawable, Size, Position> draw_entities = world.query_builder<Drawable, Size, Position>().cached().build();
    world.entity().set<Position>(Position{0,0}).set<Size>(Size{8,8}).set<Drawable>(Drawable{0xFF,0xFF,0xFF,0xFF}).set<Velocity>(Velocity{1,1});
    world.system<Position, Velocity>().each([](Position& p, Velocity& v) {
        p.x += v.x;
        p.y += v.y;
    });
    ImGuiContext *ctx = ImGui::CreateContext();
    bool sim_running = true;

    SDL_Window* window = SDL_CreateWindow("evolution!", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (window == nullptr) {
        printf("SDL Window error: %s\n", SDL_GetError());
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == nullptr) {
        printf("SDL Renderer error: %s\n", SDL_GetError());
    }
    SDL_Event event;
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
    while(sim_running) {
        SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
        //SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        //ImGui::ShowDemoWindow();
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            switch(event.type) {
                case SDL_EVENT_QUIT:
                    sim_running = false;
                    break;
            }
        }
        ImGui::Render();
        SDL_RenderClear(renderer);
        draw_entities.each([renderer](Drawable& d, Size& s, Position& p) {
            SDL_FRect rect = (SDL_FRect{p.x,p.y,s.w,s.h});
            const SDL_FRect* rp = &rect;
            SDL_SetRenderDrawColor(renderer, d.r,d.g,d.b,d.a);
            SDL_RenderFillRect(renderer, rp);

        });
        Uint64 current_tick = SDL_GetTicks();
        if (current_tick - last_physics_frame >= 1 * 1000 / MAX_PHYSICS_FPS) {
            printf("%f, %llu\n", world.delta_time() * 1000, current_tick - last_physics_frame);
            world.progress();
            last_physics_frame = current_tick;
        }
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }
    return cleanup(window, renderer, ctx);
}
void init()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("err: %s", SDL_GetError());
    }
    IMGUI_CHECKVERSION();
}

int cleanup(SDL_Window* window, SDL_Renderer* renderer, ImGuiContext* ctx) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    ImGui::DestroyContext(ctx);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLRenderer3_Shutdown();
    return 0;
}