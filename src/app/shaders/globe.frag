// SPDX-License-Identifier: GPL-3.0-or-later
// Ray-traced Earth for the Mission Control globe and ground-track map.
//
// Globe mode: a perspective camera sits camDistance Earth radii from the centre, looking at
// (centerLat, centerLon). Each pixel's ray is intersected with the unit sphere and the hit point is
// rotated into Earth-fixed coordinates (x: 0°E, y: 90°E, z: North Pole) to sample equirectangular
// textures. Map mode draws the same layers as a flat equirectangular "ground track" display.
#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 viewSize;       // item size in pixels
    float centerLat;     // radians
    float centerLon;     // radians
    float camDistance;   // Earth radii from the centre
    float fovY;          // vertical field of view, radians
    vec3 sunDir;         // unit vector to the Sun, Earth-fixed
    float mapMode;       // 0 = globe, 1 = equirectangular map
    float mapZoom;       // 1 = whole world fits
    float texturesReady; // 0 until the Earth images have loaded
    float gridAlpha;     // graticule strength
};

layout(binding = 1) uniform sampler2D dayTex;
layout(binding = 2) uniform sampler2D nightTex;

const float PI = 3.14159265358979;

vec3 ecefFromView(vec3 q, float lat, float lon)
{
    vec3 f = vec3(cos(lat) * cos(lon), cos(lat) * sin(lon), sin(lat));
    vec3 e = vec3(-sin(lon), cos(lon), 0.0);
    vec3 n = vec3(-sin(lat) * cos(lon), -sin(lat) * sin(lon), cos(lat));
    return q.x * e + q.y * n + q.z * f;
}

// Samples with derivatives taken from whichever parametrisation is continuous here, which removes
// the mip-level seam that would otherwise appear along the antimeridian.
vec4 sampleSeamless(sampler2D tex, vec2 uv)
{
    vec2 uvAlt = vec2(fract(uv.x + 0.5), uv.y);
    vec2 dx = dFdx(uv);
    vec2 dy = dFdy(uv);
    vec2 dxAlt = dFdx(uvAlt);
    vec2 dyAlt = dFdy(uvAlt);
    if (dot(dxAlt, dxAlt) + dot(dyAlt, dyAlt) < dot(dx, dx) + dot(dy, dy)) {
        dx = dxAlt;
        dy = dyAlt;
    }
    return textureGrad(tex, uv, dx, dy);
}

vec3 surfaceColour(vec3 p, float lat, float lon)
{
    vec2 uv = vec2(lon / (2.0 * PI) + 0.5, 0.5 - lat / PI);
    vec3 day = sampleSeamless(dayTex, uv).rgb;
    float lights = sampleSeamless(nightTex, uv).r;

    // Before the imagery arrives, draw a quiet ocean so the globe never flashes black.
    day = mix(vec3(0.05, 0.12, 0.16), day, texturesReady);
    lights *= texturesReady;

    float mu = dot(p, sunDir); // cosine of the solar zenith angle
    float dayAmount = smoothstep(-0.105, 0.105, mu); // civil twilight is the Sun 6° below the horizon
    vec3 lit = day * (0.22 + 0.9 * clamp(mu + 0.12, 0.0, 1.0));
    vec3 night = day * vec3(0.045, 0.06, 0.10) + vec3(1.0, 0.78, 0.46) * pow(lights, 1.5) * 1.25;
    vec3 colour = mix(night, lit, dayAmount);
    colour += vec3(0.55, 0.26, 0.10) * exp(-pow(mu / 0.07, 2.0)) * 0.16; // warm terminator band
    return colour;
}

float graticule(float latDeg, float lonDeg)
{
    vec2 g = vec2(lonDeg, latDeg) / 15.0;
    vec2 w = max(fwidth(g), vec2(1e-4));
    vec2 d = abs(fract(g - 0.5) - 0.5) / w;
    return 1.0 - clamp(min(d.x, d.y), 0.0, 1.0);
}

void main()
{
    vec2 pixel = qt_TexCoord0 * viewSize;

    if (mapMode > 0.5) {
        float mapWidth = min(viewSize.x, 2.0 * viewSize.y) * mapZoom;
        float pxPerRad = mapWidth / (2.0 * PI);
        vec2 offset = pixel - 0.5 * viewSize;
        float lat = -offset.y / pxPerRad;
        float lon = centerLon + offset.x / pxPerRad;
        lon = mod(lon + PI, 2.0 * PI) - PI;
        if (abs(lat) > 0.5 * PI) {
            fragColor = vec4(0.0);
            return;
        }
        vec3 p = vec3(cos(lat) * cos(lon), cos(lat) * sin(lon), sin(lat));
        vec3 colour = surfaceColour(p, lat, lon);
        colour = mix(colour, vec3(0.62, 0.86, 1.0), graticule(degrees(lat), degrees(lon)) * gridAlpha);
        fragColor = vec4(colour, 1.0) * qt_Opacity;
        return;
    }

    // Normalised device coordinates with y up, scaled by the half-height.
    vec2 ndc = vec2(pixel.x - 0.5 * viewSize.x, 0.5 * viewSize.y - pixel.y) / (0.5 * viewSize.y);
    vec3 ro = vec3(0.0, 0.0, camDistance);
    vec3 rd = normalize(vec3(ndc * tan(0.5 * fovY), -1.0));

    float b = dot(ro, rd);
    float c = dot(ro, ro) - 1.0;
    float closest = sqrt(max(dot(ro, ro) - b * b, 0.0)); // ray's closest approach to the centre

    // Anti-aliased silhouette: coverage from the closest-approach distance.
    float aa = max(fwidth(closest), 1e-4);
    float coverage = clamp((1.0 - closest) / aa + 0.5, 0.0, 1.0);

    // Atmosphere halo just outside the limb.
    float halo = exp(-max(closest - 1.0, 0.0) * 30.0) * (1.0 - coverage);
    vec4 outside = vec4(vec3(0.30, 0.55, 1.0) * halo * 0.55, halo * 0.55);

    if (coverage <= 0.0) {
        fragColor = outside * qt_Opacity;
        return;
    }

    float h = max(b * b - c, 0.0);
    vec3 q = ro + (-b - sqrt(h)) * rd; // hit point in view space (on the unit sphere)
    vec3 p = ecefFromView(q, centerLat, centerLon);
    float lat = asin(clamp(p.z, -1.0, 1.0));
    float lon = atan(p.y, p.x);

    vec3 colour = surfaceColour(p, lat, lon);
    colour = mix(colour, vec3(0.62, 0.86, 1.0), graticule(degrees(lat), degrees(lon)) * gridAlpha * 0.6);

    // Atmospheric scattering towards the limb, brighter on the day side.
    float facing = clamp(dot(normalize(q), -rd), 0.0, 1.0);
    float rim = pow(1.0 - facing, 2.6);
    float sunlit = smoothstep(-0.25, 0.45, dot(p, sunDir));
    colour += vec3(0.32, 0.56, 1.0) * rim * (0.12 + 0.75 * sunlit);

    vec4 globe = vec4(colour, 1.0) * coverage;
    fragColor = (globe + outside * (1.0 - globe.a)) * qt_Opacity;
}
