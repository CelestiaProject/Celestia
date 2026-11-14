in vec2 texCoord;

uniform sampler2D tex;

#ifdef PRESERVE_DEPTH
uniform highp sampler2D depthTex;
#endif

#ifdef TONE_MAP
// Exposure for tone mapping.
uniform float exposure;
#endif

#ifdef CONVERT_TO_SRGB
// Apply the sRGB electro-optical transfer function (IEC 61966-2-1).
// Input is assumed to be linear light; output is gamma-encoded for display.
vec3 linearToSRGB(vec3 c)
{
    // Clip to the display range before mix(): half-float HDR overflow can
    // produce infinity, for which mixing the transfer branches yields NaN.
    c = clamp(c, vec3(0.0), vec3(1.0));
    vec3 higher = vec3(1.055) * pow(c, vec3(1.0 / 2.4)) - vec3(0.055);
    vec3 lower  = vec3(12.92) * c;
    // step(edge, x): 0 when x < edge, 1 when x >= edge
    return mix(lower, higher, step(vec3(0.0031308), c));
}
#endif

void main(void)
{
    vec4 color = texture(tex, texCoord);
#ifdef TONE_MAP
    // Exponential tone mapping to roll off HDR highlights.
    color.rgb = vec3(1.0) - exp(-exposure * color.rgb);
#endif
#ifdef CONVERT_TO_SRGB
    color.rgb = linearToSRGB(color.rgb);
#endif
    fragColor = color;
#ifdef PRESERVE_DEPTH
    ivec2 size = textureSize(depthTex, 0);
    ivec2 coord = clamp(ivec2(texCoord * vec2(size)), ivec2(0), size - ivec2(1));
    gl_FragDepth = texelFetch(depthTex, coord, 0).r;
#endif
}
