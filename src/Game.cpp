#include "Game.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <util/ImageLoader.h>
#include <util/OSUtil.h>

#include <SDL_opengles2.h>

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

std::pair<int, int> getErrorStringNumber(const std::string& errorLog)
{
    static const auto r = std::regex("\\d:(\\d+)\\((\\d+\\)).*");
    std::smatch match;
    if (std::regex_search(errorLog, match, r)) {
        assert(match.size() == 3);
        int lineNum{};
        std::istringstream(match[1].str()) >> lineNum;
        int charNum{};
        std::istringstream(match[2].str()) >> charNum;
        return {lineNum, charNum};
    }
    return {-1, -1};
}

void checkShader(GLuint object)
{
    GLint status;
    std::string str;
    glGetShaderiv(object, GL_COMPILE_STATUS, &status);
    if (status != GL_FALSE) {
        return;
    }

    std::cerr << "Failed to compile shader: ";

    GLint logLength;
    glGetShaderiv(object, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(logLength + 1, '\0');
    glGetShaderInfoLog(object, logLength, NULL, &log[0]);

    std::vector<std::string> lines;
    {
        std::stringstream ss(str);
        std::string line;
        while (std::getline(ss, line, '\n')) {
            lines.push_back(line);
        }
    }
    int lineNum = 1;
    std::stringstream ss(log);
    std::string line;
    while (std::getline(ss, line, '\n')) {
        auto [errorLineNum, charNum] = getErrorStringNumber(line);
        if (errorLineNum != -1) {
            std::cerr << line << std::endl;
            std::cerr << "> " << lines[errorLineNum - 1] << std::endl;
            for (int i = 0; i < charNum + 4; ++i) {
                std::cerr << " ";
            }
            std::cerr << "^~\n";
            break;
        } else {
            std::cerr << line << std::endl;
        }
    }
}

GLuint initShader()
{
    // Create and compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    checkShader(vertexShader);

    // Create and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    checkShader(fragmentShader);

    // Link vertex and fragment shader into shader program and use it
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    return shaderProgram;
}

void initGeometry(GLuint shaderProgram)
{
    // Create vertex buffer object and copy vertex data into it
    GLuint vbo;
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

    // Specify the layout of the shader vertex data (positions only, 3 floats)
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
}
}

#ifdef __EMSCRIPTEN__
EM_BOOL emFullscreenCallback(
    int eventType,
    const EmscriptenFullscreenChangeEvent* e,
    void* userdata)
{
    auto& game = *static_cast<Game*>(userdata);
    // game.handleFullscreenChange(e->isFullscreen, e->screenWidth, e->screenHeight);

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
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        std::exit(1);
    }

    // Create OpenGLES 2 context on SDL window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetSwapInterval(1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GLContext glc = SDL_GL_CreateContext(window);

    // Set clear color to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    { // load texture
        const auto imageData = util::loadImage("assets/textures/shinji.png");
        assert(imageData.width != 0);
    }

    GLuint shaderProgram = initShader();
    initGeometry(shaderProgram);

    prev_time = SDL_GetTicks();
}

void Game::onQuit()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Game::loop()
{
    isRunning = true;
#ifdef __EMSCRIPTEN__
    auto res = emscripten_set_fullscreenchange_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW, (void*)this, EM_TRUE, emFullscreenCallback);
    assert(res != EMSCRIPTEN_RESULT_NOT_SUPPORTED);

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
        case SDL_MOUSEBUTTONDOWN:
            printf("Mouse click\n");
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
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the vertex buffer
    glDrawArrays(GL_TRIANGLES, 0, 3);
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
}
