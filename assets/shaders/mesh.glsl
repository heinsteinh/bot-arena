#type vertex
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

layout(std140, binding = 0) uniform Camera {
    mat4 u_view;
    mat4 u_projection;
    mat4 u_viewProjection;
    vec4 u_cameraPos;
};

uniform mat4 u_transform;

out vec3 v_worldNormal;
out vec3 v_worldPos;
out vec2 v_uv;

void main() {
    vec4 world = u_transform * vec4(a_position, 1.0);
    v_worldPos = world.xyz;
    v_worldNormal = mat3(u_transform) * a_normal;
    v_uv = a_uv;
    gl_Position = u_viewProjection * world;
}

#type fragment
#version 460 core

in vec3 v_worldNormal;
in vec3 v_worldPos;
in vec2 v_uv;

uniform vec4 u_baseColor;
uniform float u_metallic;
uniform float u_roughness;
uniform sampler2D u_albedoMap;
uniform int u_hasAlbedo;

layout(location = 0) out vec4 gAlbedo;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gWorldPos;

void main() {
    vec3 albedo = u_hasAlbedo == 1
        ? texture(u_albedoMap, v_uv).rgb * u_baseColor.rgb
        : u_baseColor.rgb;
    gAlbedo = vec4(albedo, u_metallic);
    gNormal = vec4(normalize(v_worldNormal), u_roughness);
    gWorldPos = vec4(v_worldPos, 1.0);
}
