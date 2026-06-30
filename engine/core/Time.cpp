#include "engine/core/Time.hpp"

#include <SDL3/SDL.h>

namespace engine {

float Time::s_deltaTime = 0.0f;
float Time::s_totalTime = 0.0f;

void Time::update() {
  static double previous =
      static_cast<double>(SDL_GetTicksNS()) / 1'000'000'000.0;

  const double current =
      static_cast<double>(SDL_GetTicksNS()) / 1'000'000'000.0;

  s_deltaTime = static_cast<float>(current - previous);
  s_totalTime = static_cast<float>(current);

  previous = current;
}

float Time::deltaTime() { return s_deltaTime; }

float Time::totalTime() { return s_totalTime; }

}  // namespace engine