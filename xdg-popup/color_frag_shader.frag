#version 460 core
precision mediump float;

uniform vec4 color;
uniform vec2 resolution;
uniform vec4 validGeometry;
uniform vec4 geometry;

out vec4 fragColor;

void clip()
{
    // Do not discard root view.
    if (validGeometry.x == 0 && validGeometry.y == 0 &&
        validGeometry.z == 0 && validGeometry.w == 0) {
        return;
    }

    vec2 coord = gl_FragCoord.xy / resolution.xy;

    vec2 minLimit = vec2(
        max(validGeometry.x, geometry.x),
        max(validGeometry.y, geometry.y)
    ) / resolution.xy;
    vec2 maxLimit = vec2(
        validGeometry.x + validGeometry.z,
        validGeometry.y + validGeometry.w
    ) / resolution.xy;
    maxLimit.y = 1.0 - maxLimit.y;

    if (coord.x < minLimit.x || coord.y < minLimit.y) {
        discard;
    }
    if (coord.x > maxLimit.x || coord.y < maxLimit.y) {
        discard;
    }
}

void main()
{
    clip();
    fragColor = color;
}