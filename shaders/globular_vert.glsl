attribute vec3 in_Position;
attribute vec3 in_TexCoord0; // reuse it for starSize, relStarDensity and colorIndex

uniform sampler2D colorTex;
uniform mat3 m;
uniform vec3 offset;
uniform float brightness;
uniform float pixelWeight;
uniform float scale;

const float clipDistance = 100.0; // observer distance [ly] from globular, where we
                                  // start "morphing" the star-sprite sizes towards
                                  // their physical values

varying vec4 color;

void main(void)
{
    float starSize = in_TexCoord0.s;
    float relStarDensity = in_TexCoord0.t;
    float colorIndex = in_TexCoord0.p;

    vec3 p = m * in_Position.xyz;
    float br = 2.0 * brightness;

    float s = br * starSize * scale;

    // "Morph" the star-sprite sizes at close observer distance such that
    // the overdense globular core is dissolved upon closing in.
    float obsDistanceToStarRatio = length(p + offset) / clipDistance;
    gl_PointSize = s * min(obsDistanceToStarRatio, 1.0);

    color = vec4(texture2D(colorTex, vec2(colorIndex, 0.0)).rgb, min(1.0, br * (1.0 - pixelWeight * relStarDensity)));
    set_vp(vec4(p, 1.0));
}
