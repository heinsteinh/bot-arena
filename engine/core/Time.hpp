#pragma once

namespace engine {

class Time {
 public:
  static void update();

  static float deltaTime();
  static float totalTime();

 private:
  static float s_deltaTime;
  static float s_totalTime;
};

}  // namespace engine