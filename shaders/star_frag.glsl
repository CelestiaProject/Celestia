const float degree_per_px = 0.01;
const float br_limit = 1.0 / (255.0 * 12.92);
// empirical constants
const float a = 0.123;
const float k = 0.0016;

varying vec3 v_color;
varying float max_theta;
varying float pointSize;

float psf_core(float offset)
{
    // Human eye's point source function from the research by Greg Spencer et al. (1995)
    // Optimized for the central part of the PSF. The flow from the nine neighboring pixels is constant.
    // Designed for degree_per_px == 0.01.
    return 1.0 + offset * (0.2789 * offset - 1.0);
    // the second summand is allowed to be scaled to achieve a seamless transition between modes
}

float psf_glow(float offset)
{
    // Human eye's point source function from the research by Greg Spencer et al. (1995)
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
    float black_and_white = (max_theta == -1.0) ? psf_core(offset) : psf_glow(offset);
    gl_FragColor = vec4(v_color * black_and_white, 1.0); // + vec4(0.1, 0.0, 0.0, 0.0); // square for debugging
}
