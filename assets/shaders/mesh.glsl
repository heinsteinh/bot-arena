#type vertex
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec3 a_tangent;

layout(std140, binding = 0) uniform Camera {
    mat4 u_view;
    mat4 u_projection;
    mat4 u_viewProjection;
    vec4 u_cameraPos;
};

uniform mat4 u_transform;

out vec3 v_worldNormal;
out vec3 v_worldTangent;
out vec3 v_worldPos;
out vec2 v_uv;

void main() {
    vec4 world = u_transform * vec4(a_position, 1.0);
    v_worldPos = world.xyz;
    v_worldNormal = mat3(u_transform) * a_normal;
    v_worldTangent = mat3(u_transform) * a_tangent;
    v_uv = a_uv;
    gl_Position = u_viewProjection * world;
}

#type fragment
#version 460 core

in vec3 v_worldNormal;
in vec3 v_worldTangent;
in vec3 v_worldPos;
in vec2 v_uv;

layout(std140, binding = 0) uniform Camera {
    mat4 u_view;
    mat4 u_projection;
    mat4 u_viewProjection;
    vec4 u_cameraPos;
};

uniform vec4 u_baseColor;
uniform float u_metallic;
uniform float u_roughness;
uniform sampler2D u_albedoMap;
uniform int u_hasAlbedo;
uniform sampler2D u_normalMap;
uniform int u_hasNormal;
uniform sampler2D u_heightMap;
uniform int u_hasHeight;
uniform float u_heightScale;

layout(location = 0) out vec4 gAlbedo;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gWorldPos;

// Parallax Occlusion Mapping: steep ray-march (layers scaled by view angle)
// plus one interpolation between the last two layers. Height map: white =
// raised, so marched depth = 1 - height.
vec2 parallaxOcclusion(vec2 uv, vec3 viewT) {
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float n = mix(maxLayers, minLayers, clamp(abs(viewT.z), 0.0, 1.0));
    float layerDepth = 1.0 / n;
    vec2 P = viewT.xy / viewT.z * u_heightScale;   // total UV shift at depth 1
    vec2 dUV = P / n;
    float curDepth = 0.0;
    vec2 curUV = uv;
    float d = 1.0 - texture(u_heightMap, curUV).r;
    while (curDepth < d) {
        curUV -= dUV;
        d = 1.0 - texture(u_heightMap, curUV).r;
        curDepth += layerDepth;
    }
    vec2 prevUV = curUV + dUV;
    float after = d - curDepth;
    float before =
        (1.0 - texture(u_heightMap, prevUV).r) - (curDepth - layerDepth);
    float w = after / (after - before);
    return mix(curUV, prevUV, w);
}

void main() {
    vec3 Ngeo = normalize(v_worldNormal);
    vec3 T = normalize(v_worldTangent - dot(v_worldTangent, Ngeo) * Ngeo);
    mat3 TBN = mat3(T, cross(Ngeo, T), Ngeo);

    vec2 uv = v_uv;
    if (u_hasHeight == 1) {
        vec3 viewT = normalize(transpose(TBN) * (u_cameraPos.xyz - v_worldPos));
        uv = parallaxOcclusion(v_uv, viewT);
    }

    vec3 albedo = u_hasAlbedo == 1
        ? texture(u_albedoMap, uv).rgb * u_baseColor.rgb
        : u_baseColor.rgb;
    gAlbedo = vec4(albedo, u_metallic);

    vec3 N = Ngeo;
    if (u_hasNormal == 1) {
        vec3 tn = texture(u_normalMap, uv).rgb * 2.0 - 1.0;
        N = normalize(TBN * tn);
    }
    gNormal = vec4(N, u_roughness);
    gWorldPos = vec4(v_worldPos, 1.0);
}
