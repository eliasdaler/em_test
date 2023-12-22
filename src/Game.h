#include <SDL.h>

class Game {
public:
    void start();
    void onQuit();
    void loop();
    void loopIteration();

    void update(float dt);
    void draw();

private:
    bool isRunning{false};
    SDL_Window* window{nullptr};

    SDL_Renderer* renderer{nullptr};
    SDL_Texture* texture{nullptr};

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
