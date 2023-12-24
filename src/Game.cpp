#include "Game.h"
#include "SDL_video.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <util/GLUtil.h>
#include <util/ImageLoader.h>
#include <util/OSUtil.h>

#include <Platform/gl.h>

namespace
{
// Vertex shader
const GLchar* vertexSource = R"(#version 300 es

layout(location=0) in vec3 a_position; 
layout(location=1) in vec4 a_color; 
layout(location=2) in vec2 a_uv; 

out vec4 color;   
out vec2 uv;   

void main()       
{
    gl_Position = vec4(a_position, 1.0);
    color = a_color;
    uv = a_uv;
}
)";

// Fragment shader
const GLchar* fragmentSource = R"(#version 300 es
precision mediump float;

in vec4 color;
in vec2 uv;

layout(location=0) out vec4 FragColor;

uniform sampler2D tex;

void main()
{
    vec4 col = texture(tex, uv);

    // Mom: we have sRGB at home:
    FragColor = pow(col, vec4(1.f / 2.2f));
} 
)";

std::uint32_t loadShader(const char* vertexSource, const char* fragmentSource)
{
    // vertex
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    bool ok = util::printShaderCompilationErrors(vertexShader, vertexSource);
    assert(ok);

    // fragment
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    ok = util::printShaderCompilationErrors(fragmentShader, fragmentSource);
    assert(ok);

    // link
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // check linking status
    ok = util::printShaderLinkErrors(shaderProgram);
    assert(ok);

    // detach and clean-up
    glDetachShader(shaderProgram, vertexShader);
    glDetachShader(shaderProgram, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

std::uint32_t loadTexture(const char* path)
{
    const auto imageData = util::loadImage("assets/textures/shinji.png");
    assert(imageData.pixels);
    assert(imageData.channels == 4);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(
        GL_TEXTURE_2D, // target
        0, // no mipmap
        GL_SRGB8_ALPHA8, // internalformat
        imageData.width, // width
        imageData.height, // height
        0, // border
        GL_RGBA, // format
        GL_UNSIGNED_BYTE, // type
        imageData.pixels // pixels
    );

    return texture;
}

}

void Game::initGeometry()
{
    // vao
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // vbo
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    struct Vertex {
        float pos[3];
        std::uint8_t color[4];
        float uv[2];
    };
    Vertex vertices[] = {
        {.pos = {-0.5f, -0.5f, 0.f}, .color = {255, 0, 0, 255}, .uv = {0.f, 0.f}},
        {.pos = {0.5f, -0.5f, 0.f}, .color = {0, 255, 0, 255}, .uv = {1.f, 0.f}},
        {.pos = {0.5f, 0.5f, 0.f}, .color = {0, 0, 255, 255}, .uv = {1.f, 1.f}},
        {.pos = {-0.5f, 0.5f, 0.f}, .color = {255, 0, 255, 255}, .uv = {0.f, 1.f}},
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 4, vertices, GL_STATIC_DRAW);

    // ebo
    GLushort indices[] = {0, 1, 2, 2, 3, 0};
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // specify vertex layout
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
}

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
        renderWidth,
        renderHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    screenWidth = renderWidth;
    screenHeight = renderHeight;

    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        std::exit(1);
    }

#ifdef __EMSCRIPTEN__
    // Create OpenGL ES 3 context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    // Create OpenGL 3.3 context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
#endif

    glContext = SDL_GL_CreateContext(window);

#ifndef __EMSCRIPTEN__
    // Initialize GLAD
    int gl_version = gladLoaderLoadGL();
    if (!gl_version) {
        printf("Unable to load GL.\n");
        std::exit(1);
    }
#endif

    SDL_GL_MakeCurrent(window, glContext);

    texture = loadTexture("assets/textures/shinji.png");

    glGenSamplers(1, &sampler);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    shaderProgram = loadShader(vertexSource, fragmentSource);
    initGeometry();

    prev_time = SDL_GetTicks();
}

void Game::onQuit()
{
    glDeleteSamplers(1, &sampler);
    glDeleteTextures(1, &texture);
    glDeleteProgram(shaderProgram);
    glDeleteBuffers(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);

    SDL_GL_DeleteContext(glContext);
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

#ifdef __EMSCRIPTEN__
    {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        EmscriptenFullscreenChangeEvent e{};
        if (emscripten_get_fullscreen_status(&e) != EMSCRIPTEN_RESULT_SUCCESS) return;
        handleFullscreenChange(e.isFullscreen, e.screenWidth, e.screenHeight);
    }
#else
    // i3 is silly - doesn't send any events on maximize/minimize
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    screenWidth = w;
    screenHeight = h;
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

#ifndef __EMSCRIPTEN__
                switch (event.type) {
                case SDL_WINDOWEVENT: {
                    switch (event.window.type) {
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                    case SDL_WINDOWEVENT_MAXIMIZED:
                        screenWidth = event.window.data1;
                        screenHeight = event.window.data1;
                        break;
                    }
                }
                }
#endif
            }
        }

        update(dt);

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
{}

void Game::draw()
{
    // clear whole window with black color
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, screenWidth, screenHeight);
    glClearColor(0.f, 0.f, 0.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // setup new draw area
    doLetterboxing();

    // draw
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);

    // bind texture
    const GLuint unit = 0;

    auto loc = glGetUniformLocation(shaderProgram, "tex");
    glUniform1i(loc, unit);
    glBindSampler(unit, sampler);

    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    SDL_GL_SwapWindow(window);
}

void Game::handleFullscreenChange(bool isFullscreen, int newScreenWidth, int newScreenHeight)
{
    this->isFullscreen = isFullscreen;
    // printf("handle fullscreen change!\n");

    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    int nw, nh;
    if (isFullscreen) {
        nw = newScreenWidth;
        nh = newScreenHeight;
    } else {
        nw = renderWidth;
        nh = renderHeight;
    }
    screenWidth = nw;
    screenHeight = nh;

    if (w != nw && h != nh) {
        SDL_SetWindowSize(window, screenWidth, screenHeight);
    }
}

void Game::doLetterboxing()
{
    const float sw = screenWidth;
    const float sh = screenHeight;
    const float ratio = (float)renderWidth / (float)renderHeight;

    // TODO: integer scale mode

    // letterboxing
    float vp[4] = {0.f, 0.f, sw, sh};
    if (sw / sh > ratio) { // won't fit horizontally - add vertical bars
        vp[2] = sh * ratio;
        vp[0] = (sw - vp[2]) * 0.5f; // center horizontally
    } else { // won'f fit vertically - add horizonal bars
        vp[3] = sw / ratio;
        vp[1] = (sh - vp[3]) * 0.5f; // center vertically
    }

    glEnable(GL_SCISSOR_TEST);
    glScissor(vp[0], vp[1], vp[2], vp[3]);
    glViewport(vp[0], vp[1], vp[2], vp[3]);
}
