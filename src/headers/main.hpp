#pragma once

#define SDL_MAIN_HANDLED

#include <stdio.h>
#include <vector>
#include <math.h>
#include <iostream>
#include <unordered_map>
#include <variant>
#include <regex>
#include <string>

#include "flecs.h"

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "SDL3_image/SDL_image.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include "parasail.h"

#include "PerlinNoise.hpp"
//#include "SimplexNoise.h"

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int MAX_FPS = 60;
const int MAX_PHYSICS_FPS = 60;

const int WORLD_WIDTH = 100;
const int WORLD_HEIGHT = 100;
const int TILE_WIDTH = 4;
const int TILE_HEIGHT = 4;

struct Vector2 {
    float x,y;
    Vector2() : x(0), y(0) {}
    Vector2(float _x, float _y) : x(_x), y(_y) {}
    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }

    Vector2 operator+=(const Vector2& other) {
        this->x += other.x;
        this->y += other.y;
        return *this;
    }

    Vector2 operator/(float scaler) const {
        return Vector2(x / scaler, y / scaler);
    }

    Vector2 operator/(const Vector2& other) const {
        return Vector2(x / other.x, y / other.y);
    }
    
    inline bool operator==(const Vector2& other) {
        return x == other.x && y == other.y;
    }

    inline bool operator!=(const Vector2& other) {
        return !(*this == other);
    }

    Vector2 operator*(float scalar) const {
        return Vector2(x * scalar, y * scalar);
    }
};

struct Position {
    Vector2 v;
    Position() {};
    Position(Vector2 v2) { v = v2; }
};

struct Velocity {
    Vector2 v;
    Velocity() {};
    Velocity(Vector2 v2) { v = v2; }
};

struct Size {
    Vector2 v;
    Size() {};
    Size(Vector2 v2) { v = v2; }
};

struct Drawable {
    uint8_t r,g,b,a;
};

struct Food {};

enum class Textures {
    WATER,
    DIRT,
    GRASS,
    TEXTURE_COUNT
};

enum class Terrains {
    WATER,
    DIRT,
    GRASS,
    TERRAIN_TEXTURES = (int)Textures::WATER
};

const int ATLAS_TILE_WIDTH = 32;
const int ATLAS_TILE_HEIGHT = 32;

struct Sprite {
    float x;
    float y;
    int vertical_tile_count;
    int horizontal_tile_count;
};

struct TileMap {
    int width;
    int height;
    std::vector<Vector2> currents; // fluid currents in general (water or air)
    std::vector<Terrains> terrains;
    TileMap() : TileMap(WORLD_WIDTH, WORLD_HEIGHT) {}
    TileMap(int w, int h) : width(w), height(h), currents(w*h), terrains(w*h){
        for (int i = 0; i < terrains.size(); i++) {
            terrains[i] = Terrains::WATER;
        }
    }

    Vector2 GetCurrentAt(int x, int y) const {
        return currents[x + y * width];
    }

    Vector2 GetCurrentAt(Vector2 v) const {
        return currents[((int)v.x) + ((int)v.y) * width];
    }

    void CreateCurrents() {
        for(int i = 0; i < currents.size(); i++) {
            siv::PerlinNoise noise{};
            float angle = noise.noise2D(i % width, i / width) * M_PI * 2;
            //float angle = SimplexNoise::noise(i % width, i / width) * M_PI * 2;
            float speed = SDL_randf() * 2;
            //float speed = SimplexNoise::noise(1, i % width + (currents.size() + 500), i / width + (currents.size() + 500)) * 2;
            currents[i] = Vector2(cosf(angle), sinf(angle)) * speed;
            if (std::isnan(currents[i].x) || std::isnan(currents[i].y)) {
                printf("nan: invalid angle? %f invalid speed? %f\n", angle, speed);
            }
        
        }
    }

    void UpdateCurrents() {
        for (int x, y = 0; x + y * width < currents.size(); x++) {
            if (x == width) {
                x = 0;
                y++;
            }
            Vector2 average(0,0);
            int neighbors = 0;
            if (x > 0) {
                average += GetCurrentAt(x - 1, y);
                neighbors++;
                if (x + 1 < width) {
                    average += GetCurrentAt(x + 1, y);
                }   
            }
            if (y > 0) {
                average += GetCurrentAt(x, y - 1);
                neighbors++;
                if (y + 1 < height) {
                    average += GetCurrentAt(x, y + 1);
                }   
            }
            currents[x + y * width] = average / neighbors;
            if (currents[x + y * width].x >  2 || currents[x + y * width].y > 2) {
                printf("average: (%f,%f), neighbors: %d", average.x, average.y, neighbors);
            }
        }
    }

    void ApplyNoise(){
	    for (int i = 0; i < currents.size(); i++) {
            if (SDL_randf() < 0.01) {
                float angle = SDL_randf() * M_PI * 2;
	    		float speed = SDL_randf() * 0.2 + 0.1;
                currents[i] += Vector2(cosf(angle), sinf(angle)) * speed;
            }
        }
    }
};

const std::unordered_map<std::string, char> CODON_TABLE = {
    {"GCT", 'A'}, {"GCC", 'A'}, {"GCA", 'A'}, {"GCG", 'A'}, // Ala
    {"GGT", 'R'}, {"GGC", 'R'}, {"GGA", 'R'}, {"GGG", 'R'}, // Arg
    {"AAT", 'N'}, {"AAC", 'N'}, // Asn
    {"GAT", 'D'}, {"GAC", 'D'}, // Asp
    {"TGT", 'C'}, {"TGC", 'C'}, // Cys
    {"CAA", 'Q'}, {"CAG", 'Q'}, // Gln
    {"GAA", 'E'}, {"GAG", 'E'}, // Glu
    {"GGT", 'G'}, {"GGC", 'G'}, {"GGA", 'G'}, {"GGG", 'G'}, // Gly
    {"CAT", 'H'}, {"CAC", 'H'}, // His
    {"ATT", 'I'}, {"ATC", 'I'}, {"ATA", 'I'}, // Ile
    {"TTA", 'L'}, {"TTG", 'L'}, {"CTT", 'L'}, {"CTC", 'L'}, {"CTA", 'L'}, {"CTG", 'L'}, // Leu
    {"AAA", 'K'}, {"AAG", 'K'}, // Lys
    {"ATG", 'M'}, // Met (start codon)
    {"TTT", 'F'}, {"TTC", 'F'}, // Phe
    {"CCT", 'P'}, {"CCC", 'P'}, {"CCA", 'P'}, {"CCG", 'P'}, // Pro
    {"TCT", 'S'}, {"TCC", 'S'}, {"TCA", 'S'}, {"TCG", 'S'}, {"AGT", 'S'}, {"AGC", 'S'}, // Ser
    {"ACT", 'T'}, {"ACC", 'T'}, {"ACA", 'T'}, {"ACG", 'T'}, // Thr
    {"TGG", 'W'}, // Trp
    {"TAT", 'Y'}, {"TAC", 'Y'}, // Tyr
    {"GTT", 'V'}, {"GTC", 'V'}, {"GTA", 'V'}, {"GTG", 'V'}, // Val
    {"TAA", '*'}, {"TAG", '*'}, {"TGA", '*'} // Stop codons
};

const std::unordered_map<char, std::array<double, 4>> AMINO_ACID_PROPERTIES = {
    {'A', {0.62, 0.0, 0.0, 0.31}},  // Alanine: hydrophobic, neutral, nonpolar, small
    {'R', {-2.53, 1.0, 1.0, 0.98}}, // Arginine: hydrophilic, positive, polar, large
    {'N', {-0.78, 0.0, 1.0, 0.58}}, // Asparagine: hydrophilic, neutral, polar, medium
    {'D', {-0.90, -1.0, 1.0, 0.54}}, // Aspartic acid: hydrophilic, negative, polar, medium
    {'C', {0.29, 0.0, 0.0, 0.46}},  // Cysteine: slightly hydrophobic, neutral, nonpolar, small
    {'Q', {-0.85, 0.0, 1.0, 0.68}}, // Glutamine: hydrophilic, neutral, polar, medium
    {'E', {-0.74, -1.0, 1.0, 0.68}}, // Glutamic acid: hydrophilic, negative, polar, medium
    {'G', {0.48, 0.0, 0.0, 0.0}},   // Glycine: neutral, neutral, nonpolar, smallest
    {'H', {-0.40, 0.5, 1.0, 0.78}}, // Histidine: hydrophilic, partially positive, polar, large
    {'I', {1.38, 0.0, 0.0, 0.73}},  // Isoleucine: hydrophobic, neutral, nonpolar, large
    {'L', {1.06, 0.0, 0.0, 0.73}},  // Leucine: hydrophobic, neutral, nonpolar, large
    {'K', {-1.50, 1.0, 1.0, 0.85}}, // Lysine: hydrophilic, positive, polar, large
    {'M', {0.64, 0.0, 0.0, 0.78}},  // Methionine: hydrophobic, neutral, nonpolar, large
    {'F', {1.19, 0.0, 0.0, 0.85}},  // Phenylalanine: hydrophobic, neutral, nonpolar, large
    {'P', {0.12, 0.0, 0.0, 0.51}},  // Proline: neutral, neutral, nonpolar, medium
    {'S', {-0.18, 0.0, 1.0, 0.38}}, // Serine: hydrophilic, neutral, polar, small
    {'T', {-0.05, 0.0, 1.0, 0.46}}, // Threonine: hydrophilic, neutral, polar, small
    {'W', {0.81, 0.0, 0.0, 1.0}},   // Tryptophan: hydrophobic, neutral, nonpolar, largest
    {'Y', {0.26, 0.0, 1.0, 0.85}},  // Tyrosine: hydrophobic, neutral, polar, large
    {'V', {1.08, 0.0, 0.0, 0.61}},  // Valine: hydrophobic, neutral, nonpolar, medium
    {'X', {0.0, 0.0, 0.0, 0.5}}     // Unknown: neutral values
};

// Regulatory elements
struct Promoter {
    std::string sequence;
    double strength;        // 0.0 - 1.0, how strongly it promotes transcription
    double specificity;     // 0.0 - 1.0, how specific the binding is
    
    Promoter(const std::string& seq, double str = 0.5, double spec = 0.5) 
        : sequence(seq), strength(str), specificity(spec) {}
};

struct Inhibitor {
    std::string sequence;
    double inhibition_strength;  // 0.0 - 1.0, how much it reduces expression
    double binding_affinity;     // 0.0 - 1.0, how likely to bind
    
    Inhibitor(const std::string& seq, double inh = 0.5, double aff = 0.5)
        : sequence(seq), inhibition_strength(inh), binding_affinity(aff) {}
};

struct GenomeFragment {
    std::string nucleotides;
    enum class GenomeFragmentType {
        PLASMID,
        CHROMOSOME
    };
    GenomeFragmentType type;

};

struct Genome {
    std::vector<GenomeFragment> fragments;
};

void init();
int cleanup(SDL_Window* window, SDL_Renderer* renderer, ImGuiContext* ctx);
SDL_FRect ReadAtlas(Sprite s);