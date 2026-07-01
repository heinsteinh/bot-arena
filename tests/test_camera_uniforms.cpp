#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/renderer/CameraUniforms.hpp"

using engine::makeCameraUniforms;

TEST_CASE("makeCameraUniforms with identity view", "[camera]") {
  const glm::mat4 proj =
      glm::perspective(glm::radians(60.0f), 1.6f, 0.1f, 100.0f);
  const engine::CameraUniforms u = makeCameraUniforms(glm::mat4(1.0f), proj);
  REQUIRE(u.viewProjection == proj);
  REQUIRE(u.cameraPosition.x == 0.0f);
  REQUIRE(u.cameraPosition.y == 0.0f);
  REQUIRE(u.cameraPosition.z == 0.0f);
  REQUIRE(u.cameraPosition.w == 1.0f);
}

TEST_CASE("makeCameraUniforms recovers the eye position from the view",
          "[camera]") {
  const glm::vec3 eye{3.0f, 4.0f, 5.0f};
  const glm::mat4 view = glm::lookAt(eye, glm::vec3(0.0f), {0.0f, 1.0f, 0.0f});
  const glm::mat4 proj =
      glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 50.0f);
  const engine::CameraUniforms u = makeCameraUniforms(view, proj);

  REQUIRE(u.viewProjection == proj * view);
  REQUIRE(u.cameraPosition.x == Catch::Approx(eye.x).margin(1e-3));
  REQUIRE(u.cameraPosition.y == Catch::Approx(eye.y).margin(1e-3));
  REQUIRE(u.cameraPosition.z == Catch::Approx(eye.z).margin(1e-3));
}

TEST_CASE("makeCameraUniforms fills the inverse view-projection", "[camera]") {
  const glm::vec3 eye{2.0f, 3.0f, 6.0f};
  const glm::mat4 view = glm::lookAt(eye, glm::vec3(0.0f), {0.0f, 1.0f, 0.0f});
  const glm::mat4 proj =
      glm::perspective(glm::radians(50.0f), 1.6f, 0.1f, 100.0f);
  const engine::CameraUniforms u = makeCameraUniforms(view, proj);

  const glm::mat4 id = u.invViewProjection * u.viewProjection;
  for (int c = 0; c < 4; ++c)
    for (int r = 0; r < 4; ++r)
      REQUIRE(id[c][r] == Catch::Approx(c == r ? 1.0f : 0.0f).margin(1e-3));
}
