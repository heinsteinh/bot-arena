#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>

#include "engine/core/DebugOverlay.hpp"

using engine::DebugStats;
using engine::formatDebugLines;

TEST_CASE("formatDebugLines renders the expected HUD lines", "[debug]") {
  DebugStats s;
  s.fps = 60;
  s.frameMs = 16.6f;
  s.width = 1280;
  s.height = 720;
  s.cameraPos = {0.0f, 5.0f, 12.0f};
  s.cameraFwd = {0.0f, -0.4f, -0.9f};
  s.drawCount = 2048;
  s.pointLights = 16;
  s.laneCount = 6;
  s.jobDispatches = 3;
  s.jobBatches = 40;
  s.jobItems = 2048;
  s.laneBatches = {7, 7, 6, 7, 7, 6};

  const auto lines = formatDebugLines(s);
  REQUIRE(lines.size() == 9);
  REQUIRE(lines[0] == "FPS: 60 (16.6 ms)");
  REQUIRE(lines[1] == "Res: 1280x720");
  REQUIRE(lines[2] == "Cam: (0.0, 5.0, 12.0)");
  REQUIRE(lines[3] == "Fwd: (0.0, -0.4, -0.9)");
  REQUIRE(lines[4] == "Draws: 2048");
  REQUIRE(lines[5] == "Lights: 16");
  REQUIRE(lines[6] == "Lanes: 6");
  REQUIRE(lines[7] == "Jobs: 3 disp / 40 batch / 2048 items");
  REQUIRE(lines[8] == "LaneBatches: 7 7 6 7 7 6");
}

TEST_CASE("formatDebugLines handles no lanes", "[debug]") {
  DebugStats s;
  s.laneBatches = {};
  const auto lines = formatDebugLines(s);
  REQUIRE(lines.size() == 9);
  REQUIRE(lines[8] == "LaneBatches:");
}
