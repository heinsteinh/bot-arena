#ifndef ENGINE_CORE_FIXEDTIMESTEP_HPP
#define ENGINE_CORE_FIXEDTIMESTEP_HPP

namespace engine {

struct FixedStep {
  int steps;
  float remainder;
};

// How many fixed `step`-second updates to run for `accumulated` seconds, capped
// at `maxSteps` (dropping the backlog to avoid the spiral of death).
inline FixedStep fixedTimestep(float accumulated, float step, int maxSteps) {
  int steps = static_cast<int>(accumulated / step);
  float remainder = accumulated - static_cast<float>(steps) * step;
  if (steps > maxSteps) {
    steps = maxSteps;
    remainder = 0.0f;
  }
  if (steps < 0) {
    steps = 0;
    remainder = accumulated;
  }
  return {steps, remainder};
}

}  // namespace engine

#endif  // ENGINE_CORE_FIXEDTIMESTEP_HPP
