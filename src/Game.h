#include <SDL.h>

class Game {
public:
    void start();
    void onQuit();
    void loop();
    void loopIteration();

    void update(float dt);
    void draw();

    void handleFullscreenChange(bool isFullscreen, int screenWidth, int screenHeight);

private:
    bool isRunning{false};
    SDL_Window* window{nullptr};

    SDL_Renderer* renderer{nullptr};
    SDL_Texture* texture{nullptr};

    static constexpr float FPS = 60.f;
    static constexpr float dt = 1.f / FPS;

    float accumulator = dt; // so that we get at least 1 update before render

    uint32_t prev_time = 0;

    static const int SCREEN_WIDTH = 640;
    static const int SCREEN_HEIGHT = 480;

    bool isFullscreen{false};
};
