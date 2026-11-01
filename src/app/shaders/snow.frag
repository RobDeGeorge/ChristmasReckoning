// SPDX-License-Identifier: GPL-3.0-or-later
// Procedural snowfall: three parallax layers of soft flakes drifting down and sideways.
// Stateless and cheap (no particles); time = 0 gives a still frame for reduced-motion users.
#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 viewSize;
    float time;
    float density;
};

float hash(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float layer(vec2 px, float cell, float speed, float drift, float size, float seed)
{
    vec2 p = px / cell;
    p.y -= time * speed;
    p.x += sin(time * 0.35 + p.y * 0.8 + seed) * drift;
    vec2 id = floor(p);
    vec2 f = fract(p) - 0.5;
    float h = hash(id + seed);
    if (h > density)
        return 0.0;
    vec2 jitter = vec2(hash(id + seed + 1.7), hash(id + seed + 9.1)) - 0.5;
    float d = length(f - jitter * 0.6);
    return smoothstep(size, size * 0.25, d) * (0.45 + 0.55 * hash(id + 3.3));
}

void main()
{
    vec2 px = qt_TexCoord0 * viewSize;
    float a = layer(px, 140.0, 0.30, 0.35, 0.030, 1.0)
            + layer(px, 90.0, 0.42, 0.25, 0.034, 7.0) * 0.8
            + layer(px, 55.0, 0.60, 0.15, 0.040, 13.0) * 0.55;
    a = clamp(a, 0.0, 1.0) * 0.75;
    fragColor = vec4(vec3(0.96, 0.97, 1.0) * a, a) * qt_Opacity;
}
