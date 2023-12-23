#include "Game.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <util/ImageLoader.h>
#include <util/OSUtil.h>

bool resized = false;

bool triggered = false;

#ifdef __EMSCRIPTEN__
static EM_BOOL onCanvasSizeChanged(
    int /* event_type */,
    const EmscriptenUiEvent* /* ui_event */,
    void* user_data)
{
    double canvas_width, canvas_height;
    emscripten_get_element_css_size("#canvas", &canvas_width, &canvas_height);
    printf("new size: %d, %d\n", (int)canvas_width, (int)canvas_height);
    return false;
}

EM_BOOL emFullscreenCallback(
    int eventType,
    const EmscriptenFullscreenChangeEvent* fullscreenChangeEvent,
    void* userData)
{
    triggered = true;
    printf("FULL SCREEN!\n");
    if (eventType == EMSCRIPTEN_EVENT_FULLSCREENCHANGE) {
        return EM_TRUE;
    }

    return EM_FALSE;
}
#endif

void Game::start()
{
#ifndef __EMSCRIPTEN__
    util::setCurrentDirToExeDir();
#endif
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        std::exit(1);
    }
    window = SDL_CreateWindow(
        "SDL Test",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN);

    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        std::exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    { // load texture
        const auto imageData = util::loadImage("assets/textures/shinji.png");
        assert(imageData.width != 0);

        constexpr auto rmask = 0x000000FF;
        constexpr auto gmask = 0x0000FF00;
        constexpr auto bmask = 0x00FF0000;
        constexpr auto amask = 0xFF000000;
        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
            imageData.pixels,
            imageData.width,
            imageData.height,
            32,
            4 * imageData.width,
            rmask,
            gmask,
            bmask,
            amask);
        assert(surface);
        texture = SDL_CreateTextureFromSurface(renderer, surface);

        assert(texture);

        SDL_FreeSurface(surface);
    }

    prev_time = SDL_GetTicks();
}

void Game::onQuit()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Game::loop()
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

    EMSCRIPTEN_RESULT res;

    res = emscripten_set_resize_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, EM_TRUE, onCanvasSizeChanged);
    assert(res != EMSCRIPTEN_RESULT_NOT_SUPPORTED);

    res = emscripten_set_fullscreenchange_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, EM_TRUE, emFullscreenCallback);
    assert(res != EMSCRIPTEN_RESULT_NOT_SUPPORTED);
#else
    while (isRunning) {
        loopIteration();
    }
#endif
}

void Game::loopIteration()
{
    if (!isRunning) {
        onQuit();
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        return;
    }

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            isRunning = false;
            break;
        case SDL_MOUSEBUTTONDOWN:
            printf("Mouse click\n");
            break;
        }
    }

#ifdef __EMSCRIPTEN__
    if (resized) {
        double w, h;
        emscripten_get_element_css_size("#canvas", &w, &h);
        printf("size: %d, %d\n", (int)w, (int)h);
        resized = false;
    }
#endif

#ifdef __EMSCRIPTEN__
    {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        EmscriptenFullscreenChangeEvent e{};
        if (emscripten_get_fullscreen_status(&e) != EMSCRIPTEN_RESULT_SUCCESS) return;
        int nw, nh;
        if (e.isFullscreen) {
            nw = e.screenWidth;
            nh = e.screenHeight;
        } else {
            nw = SCREEN_WIDTH;
            nh = SCREEN_HEIGHT;
        }
        if (w != nw && h != nh) {
            SDL_SetWindowSize(window, nw, nh);
        }
    }
#endif

    // Fix your timestep! game loop
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
}

void Game::update(float dt)
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

void Game::draw()
{
    if (triggered) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderClear(renderer);
        return;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    // Render texture to screen
    SDL_RenderCopy(renderer, texture, NULL, NULL);

    SDL_Rect rect;
    rect.x = (int)posX;
    rect.y = 150 + 50 * std::sin(0.1 * frameNum);
    rect.w = 200;
    rect.h = 200;

    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
    SDL_RenderFillRect(renderer, &rect);

    rect.x = (int)posX - 100;
    rect.y = 50 + 30 * std::cos(0.1 * frameNum);
    rect.w = 30;
    rect.h = 30;

    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderFillRect(renderer, &rect);

    SDL_RenderPresent(renderer);
}
