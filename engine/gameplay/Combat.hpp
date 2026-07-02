#ifndef ENGINE_GAMEPLAY_COMBAT_HPP
#define ENGINE_GAMEPLAY_COMBAT_HPP

namespace engine {

// Clamp current+delta to [0, max]. delta<0 damages, delta>0 heals.
inline float adjustHealth(float current, float delta, float max) {
  const float v = current + delta;
  if (v < 0.0f) return 0.0f;
  if (v > max) return max;
  return v;
}

// True when the agent is alive and at/under the flee fraction of max health.
inline bool shouldFlee(float current, float max, float fleeFraction) {
  return current > 0.0f && current <= max * fleeFraction;
}

}  // namespace engine

#endif  // ENGINE_GAMEPLAY_COMBAT_HPP
