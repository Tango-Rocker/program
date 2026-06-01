#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include <SDL_video.h>

static bool init_sdl(SDL_Window **window, SDL_Renderer **renderer)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    if (TTF_Init() != 0) {
        SDL_Log("TTF_Init failed: %s", TTF_GetError());
        SDL_Quit();
        return false;
    }

    const int image_flags = IMG_INIT_PNG;
    if ((IMG_Init(image_flags) & image_flags) != image_flags) {
        SDL_Log("IMG_Init failed: %s", IMG_GetError());
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    *window = SDL_CreateWindow(
        "SDL Program",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        960,
        540,
        SDL_WINDOW_SHOWN);
    if (*window == NULL) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (*renderer == NULL) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(*window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    return true;
}

static void draw_frame(SDL_Renderer *renderer, int width, int height, Uint32 ticks)
{
    SDL_SetRenderDrawColor(renderer, 18, 18, 22, 255);
    SDL_RenderClear(renderer);

    const int size = 96;
    const int x = (width - size) / 2;
    const int y = (height - size) / 2;
    const Uint8 pulse = (Uint8)(96 + (ticks / 8) % 128);
    SDL_Rect box = {x, y, size, size};

    SDL_SetRenderDrawColor(renderer, pulse, 190, 120, 255);
    SDL_RenderFillRect(renderer, &box);

    SDL_SetRenderDrawColor(renderer, 240, 240, 232, 255);
    SDL_RenderDrawRect(renderer, &box);

    SDL_RenderPresent(renderer);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    if (!init_sdl(&window, &renderer)) {
        return 1;
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }

        int width = 0;
        int height = 0;
        SDL_GetRendererOutputSize(renderer, &width, &height);
        draw_frame(renderer, width, height, SDL_GetTicks());
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0;
}
