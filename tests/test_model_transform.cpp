#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "engine/assets/MeshBounds.hpp"
#include "engine/scene/Components.hpp"
#include "engine/scene/ModelComponent.hpp"
#include "engine/scene/ModelTransform.hpp"

using engine::AABB;
using engine::ModelComponent;
using engine::modelRenderTransform;
using engine::TransformComponent;

// The old shooter proxy collapsed each submesh to
//   translate(pos + rot*(-uni*center)) * mat4_cast(rot) * scale(uni),
// with rot = ownerRot * offset and uni = ownerScale.x / largestExtent.
// modelRenderTransform must reproduce that exactly.
TEST_CASE("modelRenderTransform matches the collapsed proxy formula",
          "[model]") {
  TransformComponent t;
  t.translation = {2.0f, 0.4f, -1.0f};
  t.rotation = glm::angleAxis(0.7f, glm::vec3(0, 1, 0));
  t.scale = glm::vec3(1.1f);
  ModelComponent mc;
  mc.normalized = true;
  mc.rotationOffset = glm::angleAxis(3.1415927f, glm::vec3(0, 1, 0));
  AABB bounds;
  bounds.min = {-2, -1, -0.5f};
  bounds.max = {2, 1, 0.5f};

  const glm::mat4 got = modelRenderTransform(t, mc, bounds);

  const glm::vec3 extent = bounds.max - bounds.min;
  const float largest = std::max(extent.x, std::max(extent.y, extent.z));
  const float uni = t.scale.x / largest;
  const glm::vec3 center = (bounds.min + bounds.max) * 0.5f;
  const glm::quat rot = t.rotation * mc.rotationOffset;
  const glm::vec3 translation = t.translation + rot * (-uni * center);
  const glm::mat4 ref = glm::translate(glm::mat4(1.0f), translation) *
                        glm::mat4_cast(rot) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(uni));

  for (int c = 0; c < 4; ++c)
    for (int r = 0; r < 4; ++r)
      REQUIRE(got[c][r] == Catch::Approx(ref[c][r]).margin(1e-5f));
}

TEST_CASE("modelRenderTransform without normalize is translate*rotate*scale",
          "[model]") {
  TransformComponent t;
  t.translation = {1.0f, 2.0f, 3.0f};
  t.rotation = glm::angleAxis(0.3f, glm::vec3(0, 1, 0));
  t.scale = glm::vec3(2.0f);
  ModelComponent mc;
  mc.normalized = false;  // identity rotationOffset by default
  AABB bounds;            // ignored when !normalized
  bounds.min = {-5, -5, -5};
  bounds.max = {5, 5, 5};

  const glm::mat4 got = modelRenderTransform(t, mc, bounds);
  const glm::mat4 ref = glm::translate(glm::mat4(1.0f), t.translation) *
                        glm::mat4_cast(t.rotation) *
                        glm::scale(glm::mat4(1.0f), t.scale);

  for (int c = 0; c < 4; ++c)
    for (int r = 0; r < 4; ++r)
      REQUIRE(got[c][r] == Catch::Approx(ref[c][r]).margin(1e-5f));
}
