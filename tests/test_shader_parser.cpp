#include <catch2/catch_test_macros.hpp>

#include "engine/renderer/Shader.hpp"

using engine::Shader;

TEST_CASE("ParseSources splits #type sections", "[shader]") {
  const std::string src =
      "#type vertex\n"
      "void main() { gl_Position = vec4(0.0); }\n"
      "#type fragment\n"
      "void main() { }\n";
  const engine::ShaderSources out = Shader::ParseSources(src);
  REQUIRE(out.vertex.find("gl_Position") != std::string::npos);
  REQUIRE(out.fragment.find("void main") != std::string::npos);
  REQUIRE(out.vertex.find("#type") == std::string::npos);
  REQUIRE(out.fragment.find("gl_Position") == std::string::npos);
}
