const float degree_per_px = 0.05;
const float br_limit = 1.0 / (255.0 * 12.92);
// empirical constants
const float a = 0.123;
const float k = 0.0016;

varying vec3 v_color;
varying float max_theta;
varying float pointSize;
varying float br0;

// py: def PSF_Bounded(theta: float, max_theta: float, br_center: float):
// max_theta is common for all pixels so it's set via `varying` in the vertex shader
float psf_bounded(float theta, float br_center)
{
    // Human eye's point source function from the research by Greg Spencer et al., optimized to fit a square.
    // Lower limit on brightness and angular size: 1 Vega and 0.05 degrees per pixel. No upper limits.

    // py: if theta == 0:
    if (theta == 0)
    // py:  return br_center
        return br_center; // the center is always overexposed (zero division error)

    // py:  elif theta < max_theta:
    if (theta < max_theta)
    {
        // py: brackets = max_theta / theta - 1
        float brackets = max_theta / theta - 1.0;
        // py: return k * brackets * brackets
        return k * brackets * brackets;
    }

    // py: return 0. # after max_theta function starts to grow again
    return 0.0; // after max_theta function starts to grow again
}

void main(void)
{
    vec3 glow_colored;
    if (max_theta == -1.0)
    {
        // just a one pixel star
        glow_colored = v_color;
        // py: arr[center[1], center[0]] += scaled_color
        gl_FragColor = vec4(glow_colored, 1.0);
    }
    else
    {
        // Option 2: glare square render
        // py: theta = np.sqrt(xx*xx + yy*yy) * degree_per_px # array of distances to the center
        // in fragment shader all points have virtual dimension 1x1, so gl_PointCoord has a value from [0; 1]
        vec2 offset = (gl_PointCoord.xy - vec2(0.5)) * pointSize;
        float theta = length(offset) * degree_per_px;
        // py: glow_bw = PSF_Bounded(theta, max_theta, br0) # in the [0, 1] range, like in Celestia
        float glow_bw = psf_bounded(theta, br0);
        // py: glow_colored = color * np.repeat(np.expand_dims(glow_bw, axis=2), 3, axis=2) # scaling
        glow_colored = v_color * glow_bw;
        // py: arr[center[1]+y_min:center[1]+y_max, center[0]+x_min:center[0]+x_max] += glow_colored
        gl_FragColor = vec4(glow_colored, 1.0);
    }
}
