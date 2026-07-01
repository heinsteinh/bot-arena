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

layout(std140, binding = 1) uniform Light {
    mat4 u_lightViewProj;
    vec4 u_lightDir;
};

uniform mat4 u_transform;

out vec3 v_normal;
out vec4 v_lightClip;

void main() {
    v_normal = mat3(u_transform) * a_normal;
    vec4 world = u_transform * vec4(a_position, 1.0);
    v_lightClip = u_lightViewProj * world;
    gl_Position = u_viewProjection * world;
}

#type fragment
#version 460 core

in vec3 v_normal;
in vec4 v_lightClip;

layout(std140, binding = 1) uniform Light {
    mat4 u_lightViewProj;
    vec4 u_lightDir;
};

uniform vec4 u_baseColor;
uniform sampler2DShadow u_shadowMap;

out vec4 fragColor;

float shadowFactor(float ndl) {
    vec3 p = v_lightClip.xyz / v_lightClip.w;   // -> [-1,1]
    p = p * 0.5 + 0.5;                          // -> [0,1]
    if (p.z > 1.0) return 0.0;                  // beyond far plane: lit
    float bias = max(0.0025 * (1.0 - ndl), 0.0008);
    float texel = 1.0 / 2048.0;
    float sum = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec3 uvz = vec3(p.xy + vec2(x, y) * texel, p.z - bias);
            sum += texture(u_shadowMap, uvz);
        }
    }
    return 1.0 - sum / 9.0;                      // 1 = fully shadowed
}

void main() {
    vec3 N = normalize(v_normal);
    vec3 L = normalize(u_lightDir.xyz);
    float ndl = max(dot(N, L), 0.0);
    float shadow = shadowFactor(ndl);
    float ambient = 0.25;
    fragColor = vec4(u_baseColor.rgb * (ambient + (1.0 - shadow) * ndl),
                     u_baseColor.a);
}
