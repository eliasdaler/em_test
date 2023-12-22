#include <SDL.h>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

class Game {
public:
    void start()
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
            std::exit(1);
        }
        // Create window
        auto* window = SDL_CreateWindow(
            "SDL Test",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN);
        if (window == NULL) {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
            std::exit(1);
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

        prev_time = SDL_GetTicks();
    }

    void onQuit()
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void loop()
    {
        isRunning = true;
#ifdef __EMSCRIPTEN__
        emscripten_set_main_loop_arg(
            [](void* userdata) {
                auto* game = static_cast<Game*>(userdata);
                assert(game);
                game->loopIteration();
            },
            (void*)this,
            0,
            1);
#else
        while (isRunning) {
            loopIteration();
        }
#endif
    }

    void loopIteration()
    {
        if (!isRunning) {
            onQuit();
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#else
            std::exit(0);
#endif
        }

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) isRunning = false;
        }

        uint32_t new_time = SDL_GetTicks();
        const auto frame_time = (new_time - prev_time) / 1000.f;
        accumulator += frame_time;
        prev_time = new_time;

        if (accumulator > 10 * dt) { // game stopped for debug
            accumulator = dt;
        }

        while (accumulator >= dt) {
            { // event processing
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        isRunning = false;
                    }
                }
            }

            // update
            update(dt);

            frameNum += 1.f;

            accumulator -= dt;
        }

        draw();

#ifndef __EMSCRIPTEN__
        // Delay to not overload the CPU
        const auto frameTime = (SDL_GetTicks() - prev_time) / 1000.f;
        if (dt > frameTime) {
            SDL_Delay(dt - frameTime);
        }
#endif

        draw();
    }

    void update(float dt)
    {
        float speed = 100;
        if (moveRight) {
            posX += speed * dt;
        } else {
            posX -= speed * dt;
        }
        if (posX > 400) {
            moveRight = false;
        } else if (posX < 200) {
            moveRight = true;
        }
    }

    void draw()
    {
        SDL_RenderClear(renderer);

        SDL_Rect rect;
        rect.x = (int)posX;
        rect.y = 150 + 50 * std::sin(0.1 * frameNum);
        rect.w = 200;
        rect.h = 200;

        SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
        SDL_RenderDrawRect(renderer, &rect);

        rect.x = (int)posX - 100;
        rect.y = 50 + 30 * std::cos(0.1 * frameNum);
        rect.w = 20;
        rect.h = 20;

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderDrawRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        SDL_RenderPresent(renderer);
    }

private:
    bool isRunning{false};
    SDL_Window* window{nullptr};
    SDL_Renderer* renderer{nullptr};

    // Fix your timestep! game loop
    static constexpr float FPS = 60.f;
    static constexpr float dt = 1.f / FPS;

    float accumulator = dt; // so that we get at least 1 update before render

    bool moveRight = true;
    float posX = 250;

    float frameNum = 0;

    uint32_t prev_time = 0;

    static const int SCREEN_WIDTH = 640;
    static const int SCREEN_HEIGHT = 480;
};

int main(int argc, char* args[])
{
    Game game;
    game.start();
    game.loop();

    return 0;
}
