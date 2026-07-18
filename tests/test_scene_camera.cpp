#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/scene/Components.hpp"
#include "engine/scene/SceneCamera.hpp"

using engine::cameraTransformFromView;
using engine::lookAtTransform;
using engine::TransformComponent;
using engine::viewMatrix;

namespace {
void checkReproducesLookAt(const glm::vec3& eye, const glm::vec3& target) {
  const TransformComponent t = lookAtTransform(eye, target);
  // translation is the eye exactly
  CHECK(t.translation.x == Catch::Approx(eye.x));
  CHECK(t.translation.y == Catch::Approx(eye.y));
  CHECK(t.translation.z == Catch::Approx(eye.z));
  // viewMatrix(lookAtTransform(...)) == glm::lookAt(eye, target, +Y)
  const glm::mat4 got = viewMatrix(t);
  const glm::mat4 want = glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
  for (int c = 0; c < 4; ++c)
    for (int r = 0; r < 4; ++r)
      CHECK(got[c][r] == Catch::Approx(want[c][r]).margin(1e-4));
}
}  // namespace

TEST_CASE("lookAtTransform reproduces glm::lookAt: csm_demo camera") {
  checkReproducesLookAt({5.0f, 7.5f, 9.0f}, {-1.0f, 0.0f, -13.0f});
}
TEST_CASE("lookAtTransform reproduces glm::lookAt: normalmap camera") {
  checkReproducesLookAt({2.6f, 2.0f, 10.0f}, {0.0f, 1.6f, 0.0f});
}
TEST_CASE("lookAtTransform reproduces glm::lookAt: parallax camera") {
  checkReproducesLookAt({0.5f, 5.5f, 7.0f}, {0.0f, 0.0f, -0.5f});
}
TEST_CASE("lookAtTransform reproduces glm::lookAt: looking down +Z") {
  checkReproducesLookAt({0.0f, 1.0f, -3.0f}, {0.0f, 1.0f, 2.0f});
}

TEST_CASE("cameraTransformFromView round-trips a perspective view") {
  const glm::mat4 V =
      glm::lookAt(glm::vec3(4.0f, 3.0f, 9.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                  glm::vec3(0.0f, 1.0f, 0.0f));
  const glm::mat4 got = viewMatrix(cameraTransformFromView(V));
  for (int c = 0; c < 4; ++c)
    for (int r = 0; r < 4; ++r)
      CHECK(got[c][r] == Catch::Approx(V[c][r]).margin(1e-4));
}

TEST_CASE("cameraTransformFromView round-trips a top-down view") {
  const glm::mat4 V =
      glm::lookAt(glm::vec3(0.0f, 14.0f, 0.001f), glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, 1.0f, 0.0f));
  const glm::mat4 got = viewMatrix(cameraTransformFromView(V));
  for (int c = 0; c < 4; ++c)
    for (int r = 0; r < 4; ++r)
      CHECK(got[c][r] == Catch::Approx(V[c][r]).margin(1e-4));
}
