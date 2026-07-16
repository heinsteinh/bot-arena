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

layout(std140, binding = 1) uniform Light {
    mat4 u_cascadeViewProj[3];
    vec4 u_cascadeSplits;
    vec4 u_lightDir;
};

struct PointLight {
    vec4 positionRadius;
    vec4 color;
};
layout(std140, binding = 2) uniform PointLights {
    int u_pointCount;
    PointLight u_points[32];
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
layout(location = 3) out vec4 gShadow;

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

// Soft parallax self-shadow: march from the parallax UV toward the light,
// accumulating how far blockers rise above the ray.
float parallaxShadow(vec2 uv, vec3 lightT) {
    if (lightT.z <= 0.0) return 1.0;
    const float n = 16.0;
    const float strength = 24.0;
    float startDepth = 1.0 - texture(u_heightMap, uv).r;
    float layerDepth = startDepth / n;
    vec2 dUV = (lightT.xy / lightT.z) * u_heightScale / n;
    float curDepth = startDepth - layerDepth;
    vec2 curUV = uv + dUV;
    float occ = 0.0;
    while (curDepth > 0.0) {
        float d = 1.0 - texture(u_heightMap, curUV).r;
        if (d < curDepth) occ += (curDepth - d);
        curDepth -= layerDepth;
        curUV += dUV;
    }
    return 1.0 - clamp(occ * strength, 0.0, 1.0);
}

void main() {
    vec3 Ngeo = normalize(v_worldNormal);
    vec3 T = normalize(v_worldTangent - dot(v_worldTangent, Ngeo) * Ngeo);
    mat3 TBN = mat3(T, cross(Ngeo, T), Ngeo);

    vec2 uv = v_uv;
    vec4 selfShadow = vec4(1.0);   // r = directional, gba = point lights 0..2
    if (u_hasHeight == 1) {
        vec3 viewT = normalize(transpose(TBN) * (u_cameraPos.xyz - v_worldPos));
        uv = parallaxOcclusion(v_uv, viewT);
        vec3 lightT = normalize(transpose(TBN) * normalize(u_lightDir.xyz));
        selfShadow.r = parallaxShadow(uv, lightT);
        int np = min(u_pointCount, 3);
        for (int i = 0; i < np; ++i) {
            vec3 Lw = normalize(u_points[i].positionRadius.xyz - v_worldPos);
            vec3 Lt = normalize(transpose(TBN) * Lw);
            selfShadow[i + 1] = parallaxShadow(uv, Lt);
        }
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
    gShadow = selfShadow;
}
