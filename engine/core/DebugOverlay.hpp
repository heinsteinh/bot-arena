#ifndef ENGINE_CORE_DEBUGOVERLAY_HPP
#define ENGINE_CORE_DEBUGOVERLAY_HPP

#include <cstddef>
#include <cstdio>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace engine {

struct DebugStats {
  int fps = 0;
  float frameMs = 0.0f;
  int width = 0;
  int height = 0;
  glm::vec3 cameraPos{0.0f};
  glm::vec3 cameraFwd{0.0f};
  size_t drawCount = 0;
  int pointLights = 0;
  size_t laneCount = 0;
  size_t jobDispatches = 0;
  size_t jobBatches = 0;
  size_t jobItems = 0;
  std::vector<size_t> laneBatches;
};

// Format the debug HUD as one string per line (pure; no GL).
inline std::vector<std::string> formatDebugLines(const DebugStats& s) {
  const auto f1 = [](float v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f", v);
    return std::string(buf);
  };
  const auto vec3 = [&](const glm::vec3& v) {
    return "(" + f1(v.x) + ", " + f1(v.y) + ", " + f1(v.z) + ")";
  };

  std::vector<std::string> lines;
  lines.push_back("FPS: " + std::to_string(s.fps) + " (" + f1(s.frameMs) +
                  " ms)");
  lines.push_back("Res: " + std::to_string(s.width) + "x" +
                  std::to_string(s.height));
  lines.push_back("Cam: " + vec3(s.cameraPos));
  lines.push_back("Fwd: " + vec3(s.cameraFwd));
  lines.push_back("Draws: " + std::to_string(s.drawCount));
  lines.push_back("Lights: " + std::to_string(s.pointLights));
  lines.push_back("Lanes: " + std::to_string(s.laneCount));
  lines.push_back("Jobs: " + std::to_string(s.jobDispatches) + " disp / " +
                  std::to_string(s.jobBatches) + " batch / " +
                  std::to_string(s.jobItems) + " items");

  std::string lanes = "LaneBatches:";
  for (size_t b : s.laneBatches) lanes += " " + std::to_string(b);
  lines.push_back(lanes);
  return lines;
}

}  // namespace engine

#endif  // ENGINE_CORE_DEBUGOVERLAY_HPP
