#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdint>
#include <format>
#include <string>

#include "sim/sim_state.hpp"
#include "sim/tick_pacer.hpp"

namespace
{

// 키 -> 배속 (1장: 일시정지 / 1x / 2x / 10x / 60x). 해당 없으면 -1.
int SpeedForKey(SDL_Keycode key)
{
    switch (key)
    {
    case SDLK_1: return 1;
    case SDLK_2: return 2;
    case SDLK_3: return 10;
    case SDLK_4: return 60;
    default: return -1;
    }
}

std::string WindowTitle(const sim::SimState& state, int speed)
{
    const std::uint64_t tick = state.clock.tick;
    const std::uint64_t day = tick / 1440;
    const std::uint64_t hour = tick % 1440 / 60;
    const std::uint64_t minute = tick % 60;
    const std::string speedText = speed == 0 ? std::string("PAUSED") : std::format("x{}", speed);
    return std::format("RealRTS - Day {} {:02}:{:02} - {}", day, hour, minute, speedText);
}

}

int main(int, char**)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("RealRTS", 1280, 720, SDL_WINDOW_RESIZABLE, &window, &renderer))
    {
        SDL_Log("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    sim::SimState state;
    sim::TickPacer pacer;
    int speedBeforePause = pacer.Speed();
    std::string shownTitle;
    std::uint64_t lastNs = SDL_GetTicksNS();

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
            {
                if (event.key.key == SDLK_SPACE)
                {
                    if (pacer.Speed() == 0)
                    {
                        pacer.SetSpeed(speedBeforePause);
                    }
                    else
                    {
                        speedBeforePause = pacer.Speed();
                        pacer.SetSpeed(0);
                    }
                }
                else if (const int speed = SpeedForKey(event.key.key); speed > 0)
                {
                    pacer.SetSpeed(speed);
                }
            }
        }

        const std::uint64_t nowNs = SDL_GetTicksNS();
        const double realSeconds = static_cast<double>(nowNs - lastNs) / 1e9;
        lastNs = nowNs;

        const int ticks = pacer.Advance(realSeconds);
        for (int i = 0; i < ticks; ++i)
        {
            sim::Step(state);
        }

        std::string title = WindowTitle(state, pacer.Speed());
        if (title != shownTitle)
        {
            SDL_SetWindowTitle(window, title.c_str());
            shownTitle = std::move(title);
        }

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}