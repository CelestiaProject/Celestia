const float degree_per_px = 0.01;
const float br_limit = 1.0 / (255.0 * 12.92);
// empirical constants
const float a = 0.123;
const float k = 0.0016;

varying vec3 v_color;
varying float max_theta;
varying float pointSize;

float psf_central(float offset)
{
    // Human eye's point source function from the research by Greg Spencer et al.
    // Optimized for the central part of the PSF. The flow from the four neighboring pixels is constant.
    // Designed for degree_per_px == 0.01.
    if (offset < 1.6667)
    {
        return 1.0 + 1.136 * offset * (0.3 * offset - 1.0);
    }
    return 0.0; // function starts to grow again
}

float psf_outer(float offset)
{
    // Human eye's point source function from the research by Greg Spencer et al.
    // Optimized for the outer part of the PSF. Designed with bounds by arctangent in mind.
    // Causes star blinking with degree_per_px > 0.01, large grid misses the center peak of brightness.
    float theta = offset * degree_per_px;
    if (theta == 0)
        return 100.; // the center is always overexposed (zero division error)
    if (theta < max_theta)
    {
        float brackets = max_theta / theta - 1.0;
        return k * brackets * brackets;
    }
    return 0.0; // after max_theta function starts to grow again
}

void main(void)
{
    // in fragment shader all points have virtual dimension 1x1, so gl_PointCoord has a value from [0; 1]
    float offset = length((gl_PointCoord.xy - vec2(0.5)) * pointSize);
    float glow_bw = (max_theta == -1.0) ? psf_central(offset) : psf_outer(offset);
    vec3 glow_colored = v_color * glow_bw; // color and brightness scaling
    gl_FragColor = vec4(glow_colored, 1.0)+ vec4(0.1, 0.0, 0.0, 0.0);
}
