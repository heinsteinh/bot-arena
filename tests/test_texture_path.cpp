#include <catch2/catch_test_macros.hpp>

#include "engine/assets/TexturePath.hpp"

using engine::resolveTexturePath;

TEST_CASE("resolveTexturePath joins a relative texture to the model dir",
          "[texpath]") {
  REQUIRE(resolveTexturePath("assets/Objects/Planet/planet.obj",
                             "planet_Quom1200.png") ==
          "assets/Objects/Planet/planet_Quom1200.png");
}

TEST_CASE("resolveTexturePath keeps a nested relative path", "[texpath]") {
  REQUIRE(resolveTexturePath("a/b/model.obj", "tex/diffuse.png") ==
          "a/b/tex/diffuse.png");
}

TEST_CASE("resolveTexturePath passes absolute paths through", "[texpath]") {
  REQUIRE(resolveTexturePath("a/b/model.obj", "/abs/tex.png") ==
          "/abs/tex.png");
}

TEST_CASE("resolveTexturePath normalizes backslashes", "[texpath]") {
  REQUIRE(resolveTexturePath("a/b/model.obj", "sub\\tex.png") ==
          "a/b/sub/tex.png");
}

TEST_CASE("resolveTexturePath handles a model with no directory", "[texpath]") {
  REQUIRE(resolveTexturePath("model.obj", "tex.png") == "tex.png");
}
