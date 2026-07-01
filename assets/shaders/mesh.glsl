#type vertex
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

layout(std140, binding = 0) uniform Camera {
    mat4 u_view;
    mat4 u_projection;
    mat4 u_viewProjection;
    vec4 u_cameraPos;
};

uniform mat4 u_transform;

out vec3 v_worldNormal;
out vec3 v_worldPos;

void main() {
    vec4 world = u_transform * vec4(a_position, 1.0);
    v_worldPos = world.xyz;
    v_worldNormal = mat3(u_transform) * a_normal;
    gl_Position = u_viewProjection * world;
}

#type fragment
#version 460 core

in vec3 v_worldNormal;
in vec3 v_worldPos;

uniform vec4 u_baseColor;

layout(location = 0) out vec4 gAlbedo;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gWorldPos;

void main() {
    gAlbedo = u_baseColor;
    gNormal = vec4(normalize(v_worldNormal), 1.0);
    gWorldPos = vec4(v_worldPos, 1.0);
}
