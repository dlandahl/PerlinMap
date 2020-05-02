
#include <math.h>
#include <iostream>
#include "SDL2/SDL.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

uint16_t const screen_width = 1280;
uint16_t const screen_height = 800;
float const circle = 6.28318530;

template <typename T> struct vec2
{
    T x;
    T y;
};

float cos_interpolate(float a, float b, float ratio) {
    float mu = (1.f - cos(ratio * circle / 2.f)) / 2.f;
    return (1.0f - mu) * a + mu * b;
}

struct PerlinNoise
{
    enum GridIndex {
        top_left, top_right, bottom_left, bottom_right
    };
    
#ifdef __EMSCRIPTEN__
    int seed = emscripten_get_now();
#else
    int seed = time(NULL);
#endif  

    float get_gradient_at_cell(vec2<int16_t> cell)
    {
        uint32_t composite_seed = (cell.x + seed) ^ (cell.y * cell.x);
        srand(composite_seed);
        uint32_t r = rand();
        float val = (float) r / RAND_MAX;
        return val * circle;
    }

    vec2<int16_t> get_cell_coordinates(vec2<float> location, GridIndex cell_index)
    {
        int16_t cell_x = (int16_t) location.x;
        int16_t cell_y = (int16_t) location.y;
        if (cell_index & 0b01) cell_x++;
        if (cell_index & 0b10) cell_y++;
        return { cell_x, cell_y };
    }

    float get_dot_product(vec2<float> location, GridIndex grid_index)
    {
        vec2<int16_t> cell_location = get_cell_coordinates(location, grid_index);
        float gradient_at_grid = get_gradient_at_cell(cell_location);

        float x_distance = location.x - cell_location.x;
        float y_distance = location.y - cell_location.y;

        float x_product = x_distance * cos(gradient_at_grid);
        float y_product = y_distance * sin(gradient_at_grid);

        return x_product + y_product;
    }

    float get_perlin_value(float x, float y)
    {
        float products[4];
        for (int n = 0; n < 4; n++) products[n] = get_dot_product(vec2<float> {x, y}, (GridIndex) n);
        
        int x0 = x;
        int y0 = y;

        float sx = x - x0;
        float sy = y - y0;

        float ix0, ix1, value;

        ix0 = cos_interpolate(products[0], products[1], sx);
        ix1 = cos_interpolate(products[2], products[3], sx);

        value = cos_interpolate(ix0, ix1, sy);
        return value;
    }
};

struct Colour
{
    uint8_t r = 0, g = 0, b = 0, a = 255;
    
    Colour(float r, float g, float b) : r(255*r), g(255*g), b(255*b) {}
    Colour(int r, int g, int b) : r(r), g(g), b(b) {}
    
    Colour() = default;

    uint8_t& operator[](unsigned n)
    {
        return *((uint8_t*) this + n);
    }

    operator uint32_t() { return ( r << 24 | g << 16 | b << 8 | a ); }
};

bool run = 1;
struct LoopData
{
    SDL_Window* output_window;
    SDL_Surface* output_surface;
    SDL_Surface* surface;
};

float const scaling = 5;
void main_loop(void* void_loop_data)
{
    LoopData* loop_data = (LoopData*) void_loop_data;
    static float _x = 0, _y = 0;
    float dx = 0, dy = 0;
    SDL_Event ev;   

    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) run = 0;
        //if (ev.type == SDL_MOUSEMOTION) if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) { dx = -ev.motion.xrel; dy = -ev.motion.yrel; }
        //if (ev.type == SDL_MOUSEWHEEL) { scaling += ev.wheel.y * scaling * 0.05; if (scaling < 1) scaling = 1; }
    }
    _x += dx / scaling;
    _y += dy / scaling;
    
    SDL_Rect srcrect = { int(_x), int(_y), int(screen_width / scaling), int(screen_height / scaling) };
    SDL_FillRect(loop_data->output_surface, NULL, uint32_t(0));
    SDL_BlitScaled(loop_data->surface, NULL, loop_data->output_surface, NULL);
    SDL_UpdateWindowSurface(loop_data->output_window);
}

int main(int argc, const char ** argv)
{
    int map_width = screen_width / scaling;
    int map_height = screen_height / scaling;
    
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* output_window = SDL_CreateWindow("Map Game", 0, 0, screen_width, screen_height, SDL_WINDOW_SHOWN);
    SDL_Surface* output_surface = SDL_GetWindowSurface(output_window);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, map_width, map_height, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
    LoopData loop_data { output_window, output_surface, surface };
    
    PerlinNoise noise;

#ifndef __EMSCRIPTEN__
    if (argc > 1) {
        noise.seed = 0;
        for (char c = -1, n = 0; c != 0; n++) {
            c = argv[1][n];
            noise.seed += c;
        }
    }
#endif
    
    //SDL_FillRect(output_surface, NULL, uint32_t(0));
    //SDL_UpdateWindowSurface(output_window);

    srand(time(NULL));
    uint type = rand() % 2;
    
    for (int x = 0; x < map_width; x++) {
        for (int y = 0; y < map_height; y++) {
            Colour col = { 1.f, 1.f, 1.f };

            float val = noise.get_perlin_value(x / 100.f + 0.325, y / 100.f) * 0.5;
            val += noise.get_perlin_value(x / 20.f + 15.135, y / 20.f) * 0.25;
            val += noise.get_perlin_value(x / 5.f + 152.25235, y / 5.f) * 0.1;
            //val += (noise.get_perlin_value(x / 700.f, y / 700.f));

            switch (type) {
            case 0:
                val += cos(((float) x / map_width)) * 1.3;
                val -= 0.6;
                break;
            case 1:
                val += cos(x / 40.f - map_width / 80.f) * .3f;
                val += cos(y / 40.f - map_height / 80.f) * .3f;
                val += 0.2;
            };

            
            if (val < 0.4) col = {0.1f, 0.5, 0.7};
            else if (val < 0.5) col = {0.01f, 0.7, 0.9};
            else if (val < 0.53) col = {0.75f, 0.75f, 0.6f};
            else if (val < 0.6) col = {0.26f, 0.69f, 0.3f};
            else if (val < 0.75) col = {0.1f, 0.4f, 0.15f};
            else if (val < 0.90) col = { 0.16f, 0.14f, 0.14f };
            if (val > 1) val = 1;
            if (val < 0) val = 0;
            
            ((uint32_t*)surface->pixels)[x + (surface->pitch / 4) * y] = col;
        }
        SDL_FillRect(output_surface, NULL, uint8_t(float(x / (float)map_width) * 255.f) << 16);
        SDL_UpdateWindowSurface(output_window);
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(main_loop, &loop_data, 60, 1);
#else
    while (run) {
        main_loop(&loop_data);
    }
#endif
 
    SDL_DestroyWindow(output_window);
    SDL_Quit();
}
