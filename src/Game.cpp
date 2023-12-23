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

layout(location=0) in vec3 position; 
layout(location=1) in vec4 i_color; 

out vec4 color;   

void main()       
{
    gl_Position = vec4(position.xyz, 1.0);
    color = i_color;
}
)";

// Fragment shader
const GLchar* fragmentSource = R"(#version 300 es
precision mediump float;

in vec4 color;

layout(location=0) out vec4 FragColor;

void main()
{
    FragColor = color;
} 
)";

GLuint initShader()
{
    // Create and compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    util::printShaderErrors(vertexShader);

    // Create and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    util::printShaderErrors(fragmentShader);

    // Link vertex and fragment shader into shader program and use it
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    // check linking status
    GLint status;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        std::string msg("Program linking failure: ");
        std::cerr << "Failed to link program: ";

        GLint logLength;
        glGetShaderiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(logLength + 1, '\0');
        glGetProgramInfoLog(shaderProgram, logLength, NULL, &log[0]);
        std::cerr << log << std::endl;
        return 0;
    }

    return shaderProgram;
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
        float color[4];
    };
    Vertex vertices[] = {
        {.pos = {-0.5f, -0.5f, 0.f}, .color = {1.f, 0.f, 0.f, 1.f}},
        {.pos = {0.5f, -0.5f, 0.f}, .color = {0.f, 1.f, 0.f, 1.f}},
        {.pos = {0.5f, 0.5f, 0.f}, .color = {0.f, 0.f, 1.f, 1.f}},
        {.pos = {-0.5f, 0.5f, 0.f}, .color = {1.f, 0.f, 1.f, 1.f}},
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

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);
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

    { // load texture
      // const auto imageData = util::loadImage("assets/textures/shinji.png");
      // assert(imageData.width != 0);
    }

    shaderProgram = initShader();
    initGeometry();

    prev_time = SDL_GetTicks();
}

void Game::onQuit()
{
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
