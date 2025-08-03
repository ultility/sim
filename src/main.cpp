#include "headers/main.hpp"

using TexturedEnum = std::variant<Terrains>;

const std::unordered_map<TexturedEnum, Sprite> atlas = {
    {Terrains::WATER, Sprite{0,0,1,1}},
    {Terrains::DIRT, Sprite{1,0,1,1}},
    {Terrains::GRASS, Sprite{2,0,1,1}},
};

const Vector2 EMPTY = Vector2(0,0);


struct CurrentInteractable {};
struct Organism {
    float energy;
};

int main(int argc, char** argv) {
    //system("blastn -query ./assets/input.fasta -db ./assets/db.fasta -out ./assets/output.txt -outfmt 6");
    init();
    flecs::world world;
    TileMap m = TileMap(WORLD_WIDTH, WORLD_HEIGHT);
    m.CreateCurrents();
    world.set<TileMap>(m);
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
    SDL_Texture* Tileset = IMG_LoadTexture(renderer, "assets/tilemap.png");
    if (Tileset == nullptr) {
        printf("failed to create Tileset, %s", SDL_GetError());
    }

    Uint64 last_frame = 0, last_physics_frame = 0;
    flecs::query<Drawable, Size, Position> draw_entities = world.query_builder<Drawable, Size, Position>().cached().build();
    flecs::entity organism = world.entity().set<Organism>(Organism{100}).set<Position>(Position(Vector2(0,0))).set<Size>(Size(Vector2(8,8))).set<Drawable>(Drawable{0xFF,0xFF,0xFF,0xFF}).set<Velocity>(Velocity(Vector2(0,0))).add<CurrentInteractable>();
    world.system<Position, CurrentInteractable>().each([world] (Position& p, CurrentInteractable) {
        TileMap m = world.get<TileMap>();
        Vector2 current = m.GetCurrentAt(p.v / Vector2(TILE_WIDTH, TILE_HEIGHT));
        if (!std::isnan(current.x)) {
            p.v.x += current.x;
        }
        if (!std::isnan(current.y)) {
            p.v.y += current.y;
        }
    });
    world.system<TileMap>().each([](TileMap& t) {
        t.UpdateCurrents();
        t.ApplyNoise();
    });
    world.system("food spawner").interval(1).run_each([world](){
        world.entity()
        .add<Food>()
        .set<Position>(Position(Vector2(SDL_rand(WORLD_WIDTH * TILE_WIDTH), SDL_rand(WORLD_HEIGHT * TILE_HEIGHT))))
        .set<Drawable>(Drawable{0xFF,0x0,0x0,0xFF})
        .set<Size>(Size(Vector2(4,4)));
    });
    world.system<Position, Size>().kind(flecs::OnValidate).each([m](Position& p, Size& s) {
        if (p.v.x < 0) {
            p.v.x = 0;
        }
        else if (p.v.x + s.v.x > m.width * TILE_WIDTH) {
            p.v.x = m.width * TILE_WIDTH - 1 - s.v.x;
        }
        if (p.v.y < 0) {    
            p.v.y = 0;
        }
        else if (p.v.y + s.v.y > m.height * TILE_HEIGHT) {
            p.v.y = m.height * TILE_HEIGHT - 1 - s.v.y;
        }
    });
    flecs::query<Food, Size, Position> collison = world.query<Food, Size, Position>();
    world.system<Organism, Size, Position>().each([&collison](Organism& o, Size& o_s, Position& o_p) {
        SDL_FRect organism_rect{o_p.v.x, o_p.v.y,o_s.v.x, o_s.v.y};
        collison.each([&organism_rect, &o](flecs::entity e, Food f, const Size& f_s, const Position& f_p){
            SDL_FRect food_rect{f_p.v.x, f_p.v.y,f_s.v.x, f_s.v.y};
            if (SDL_HasRectIntersectionFloat(&organism_rect, &food_rect)) {
                e.destruct();
                o.energy += 10;
            }
        });
    });
    world.system<Position, Velocity>().each([](flecs::entity e, Position& p, Velocity& v) {
        if (v.v != EMPTY) {
            if (e.has<Organism>()) {
                p.v += v.v;
                p.v += v.v;
                e.get_mut<Organism>().energy -= 1.0 / TILE_WIDTH;
            }
        }
    });
    world.system<Organism>().each([](flecs::entity e, Organism& o) {
        if (o.energy < 0) {
            e.destruct();
        }
    });
    SDL_Event event;
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
    while(sim_running) {
        //ImGui::ShowDemoWindow();
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            switch(event.type) {
                case SDL_EVENT_QUIT:
                    sim_running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP:
                    if (organism.is_alive()) { 
                        Velocity v = organism.get<Velocity>();
                        switch (event.key.key) {
                            case SDLK_S:
                                v.v.y = event.key.down ? 1 : 0;
                                break;
                            case SDLK_W:
                                v.v.y = event.key.down ? -1 : 0;
                                break;
                            case SDLK_A:
                                v.v.x = event.key.down ? -1 : 0;
                                break;
                            case SDLK_D:
                                v.v.x = event.key.down ? 1 : 0;
                                break;
                        }
                        organism.set<Velocity>(v);
                    }
            }
        }
        Uint64 current_tick = SDL_GetTicks();
        if (current_tick - last_physics_frame >= 1 * 1000 / MAX_PHYSICS_FPS) {
            world.progress();
            last_physics_frame = current_tick;
        }
        if (current_tick - last_frame >= 1 * 1000 / MAX_FPS) {
            SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
            ImGui_ImplSDLRenderer3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
            ImGui::Begin("debug");
            if (organism.is_alive()) {
            ImGui::TextColored(ImVec4{1,1,1,1}, "energy: %f", organism.get<Organism>().energy);
            ImGui::TextColored(ImVec4{1,1,1,1}, "position: (%.2f,%.2f)", organism.get<Position>().v.x, organism.get<Position>().v.y);
            }
            SDL_RenderClear(renderer);
            for (int i = 0; i < m.terrains.size(); i++) {
                Sprite s = atlas.at(TexturedEnum(m.terrains[i]));
                const SDL_FRect src = ReadAtlas(s);
                const SDL_FRect dst{i % m.width * (float)TILE_WIDTH, i / m.width * (float)TILE_HEIGHT, (float)TILE_WIDTH, TILE_HEIGHT};
                if (dst.x + dst.w > 0 && dst.x < m.width * TILE_WIDTH && dst.y + dst.h > 0 && dst.y < m.height * TILE_HEIGHT) {
                    SDL_RenderTexture(renderer, Tileset, &src, &dst);
                }  
            }
            draw_entities.each([renderer](Drawable& d, Size& s, Position& p) {
                SDL_FRect rect = (SDL_FRect{p.v.x,p.v.y,s.v.x,s.v.y});
                const SDL_FRect* rp = &rect;
                SDL_SetRenderDrawColor(renderer, d.r,d.g,d.b,d.a);
                SDL_RenderFillRect(renderer, rp);
            });
            SDL_SetRenderDrawColor(renderer, 0xFF, 0x0, 0xFF, 0xFF);
            last_frame = current_tick;
            ImGui::End();
            ImGui::Render();
            ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
            SDL_RenderPresent(renderer);
        }
    }
    SDL_DestroyTexture(Tileset);
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
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(ctx);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 0;
}

SDL_FRect ReadAtlas(Sprite s) {
    return SDL_FRect{s.x * ATLAS_TILE_WIDTH, s.y * ATLAS_TILE_WIDTH, (float)s.horizontal_tile_count * ATLAS_TILE_WIDTH, (float)s.vertical_tile_count * ATLAS_TILE_HEIGHT};
}