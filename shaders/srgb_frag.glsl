varying vec2 texCoord;

uniform sampler2D tex;

// Apply the sRGB electro-optical transfer function (IEC 61966-2-1).
// Input is assumed to be linear light; output is gamma-encoded for display.
vec3 linearToSRGB(vec3 c)
{
    // Clamp negatives to avoid undefined pow() behaviour
    c = max(c, vec3(0.0));
    vec3 higher = vec3(1.055) * pow(c, vec3(1.0 / 2.4)) - vec3(0.055);
    vec3 lower  = vec3(12.92) * c;
    // step(edge, x): 0 when x < edge, 1 when x >= edge
    return mix(lower, higher, step(vec3(0.0031308), c));
}

void main(void)
{
    vec4 color = texture2D(tex, texCoord);
    // Clamp to [0,1] before conversion — the half-float FBO can accumulate
    // values above 1.0 from additive blending (e.g. star glow), which must
    // be saturated before the sRGB transfer function is applied.
    gl_FragColor = vec4(linearToSRGB(min(color.rgb, vec3(1.0))), color.a);
}
