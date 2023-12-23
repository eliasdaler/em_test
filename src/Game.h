#include <SDL.h>

#include <cstdint>

class Game {
public:
    void start();
    void onQuit();
    void loop();
    void loopIteration();

    void update(float dt);
    void draw();

    void handleFullscreenChange(bool isFullscreen, int screenWidth, int screenHeight);

    void initGeometry();

private:
    void doLetterboxing();

    bool isRunning{false};
    SDL_Window* window{nullptr};
    SDL_GLContext glContext{nullptr};

    SDL_Renderer* renderer{nullptr};

    static constexpr float FPS = 60.f;
    static constexpr float dt = 1.f / FPS;

    float accumulator = dt; // so that we get at least 1 update before render

    uint32_t prev_time = 0;

    static const int renderWidth = 640;
    static const int renderHeight = 480;

    bool isFullscreen{false};
    int screenWidth{0};
    int screenHeight{0};

    std::uint32_t shaderProgram;
    std::uint32_t vao;
    std::uint32_t vbo;
    std::uint32_t ebo;
};
