#type vertex
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_viewProjection;
uniform mat4 u_transform;

out vec3 v_normal;

void main() {
    v_normal = mat3(u_transform) * a_normal;
    gl_Position = u_viewProjection * u_transform * vec4(a_position, 1.0);
}

#type fragment
#version 460 core

in vec3 v_normal;
uniform vec4 u_baseColor;
out vec4 fragColor;

void main() {
    vec3 N = normalize(v_normal);
    vec3 L = normalize(vec3(0.4, 0.8, 0.3));
    float diffuse = max(dot(N, L), 0.0);
    float ambient = 0.25;
    fragColor = vec4(u_baseColor.rgb * (ambient + diffuse), u_baseColor.a);
}
