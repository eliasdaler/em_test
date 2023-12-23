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

#include <util/GLUtil.h>
#include <util/ImageLoader.h>
#include <util/OSUtil.h>

#include <Platform/gl.h>

namespace
{
// Vertex shader
const GLchar* vertexSource = R"(#version 300 es

in vec4 position; 
out vec3 color;   
void main()       
{
    gl_Position = vec4(position.xyz, 1.0);
    color = gl_Position.xyz + vec3(0.5);   
}
)";

// Fragment shader
const GLchar* fragmentSource = R"(#version 300 es
precision mediump float;

in vec3 color;
out vec4 FragColor;

void main()
{
    FragColor = vec4(color.xyz, 1);
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
    util::printShaderErrors(vertexShader);

    // Link vertex and fragment shader into shader program and use it
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    return shaderProgram;
}

}

void Game::initGeometry()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    /* clang-format off */
    GLfloat vertices[] = 
    {
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f
    };
    /* clang-format on */
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
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
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

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

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            isRunning = false;
            break;
        }
    }

#ifdef __EMSCRIPTEN__
    {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        EmscriptenFullscreenChangeEvent e{};
        if (emscripten_get_fullscreen_status(&e) != EMSCRIPTEN_RESULT_SUCCESS) return;
        handleFullscreenChange(e.isFullscreen, e.screenWidth, e.screenHeight);
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
    // Clear screen
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    SDL_GL_SwapWindow(window);
}

void Game::handleFullscreenChange(bool isFullscreen, int screenWidth, int screenHeight)
{
    this->isFullscreen = isFullscreen;
    printf("handle fullscreen change!\n");

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    int nw, nh;
    if (isFullscreen) {
        nw = screenWidth;
        nh = screenHeight;
    } else {
        nw = SCREEN_WIDTH;
        nh = SCREEN_HEIGHT;
    }
    if (w != nw && h != nh) {
        printf("SDL_SetWindowSize(%d, %d)\n", nw, nh);
        SDL_SetWindowSize(window, nw, nh);
    }
    if (isFullscreen) {
        doLetterboxing(screenWidth, screenHeight);
    } else {
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

void Game::doLetterboxing(int screenWidth, int screenHeight)
{
    GLfloat vp[4] = {0.0, 0.0, 1.0, 1.0};

    const auto irx = SCREEN_WIDTH;
    const auto iry = SCREEN_HEIGHT;

    const auto scale = std::min(screenWidth / irx, screenHeight / iry);

    // letterboxing
    float ssx = (float)screenWidth / (irx * scale);
    float ssy = (float)screenHeight / (iry * scale);

    if (ssx > ssy) { // won't fit horizontally - add vertical bars
        vp[2] = ssy / ssx;
        vp[0] = (1.0f - vp[2]) / 2.f; // center horizontally
    } else { // won'f fit vertically - add horizonal bars
        vp[3] = ssx / ssy;
        vp[1] = (1.0f - vp[3]) / 2.0f; // center vertically
    }

    // back to pixels
    glViewport(
        vp[0] * screenWidth, vp[1] * screenHeight, vp[2] * screenWidth, vp[3] * screenHeight);
}
