const float degree_per_px = 0.05;
const float br_limit = 1.0 / (255.0 * 12.92);

varying vec3 v_color;
varying vec3 v_max_tetha_hk;
varying float pointSize;

float psf_square(float theta, float min_theta, float max_theta, float h, float k, float b)
{
    // Human eye's point source function, optimized to fit a square.
    // Lower limit on brightness and angular size: 1 Vega and 0.05 degrees per pixel.
    // No upper limits.

    if (theta < min_theta)
        return 1.0; // overexposed

    if (theta < max_theta)
    {
        float brackets = b / (theta - h) - 1.0;
        return brackets * brackets / k;
    }

    return 0.0; // after max_theta function starts to grow again
}

/*
float psf_fullscreen(float theta, float min_theta):
{
    // Human eye's point source function, optimized to be a full-screen shader.
    // The price to pay for simplification is a brightness reduction compared to the original PSF.

    if (theta2 < min_theta)
        return 1; // overexposed

    return 4.43366571e-6 / theta;
}
*/

void main(void)
{
    float max_theta = v_max_tetha_hk.x;
    if (max_theta == -1.0)
    {
        gl_FragColor = vec4(v_color, 1.0);
    }
    else
    {
        float h = v_max_tetha_hk.y;
        float k = v_max_tetha_hk.z;

        float b = max_theta - h;
        float min_theta = h + b / (sqrt(k) + 1.0);

        vec2 offset = (gl_PointCoord.xy - vec2(0.5)) * pointSize;
        float theta = length(offset) * degree_per_px;

        gl_FragColor = vec4(v_color * psf_square(theta, min_theta, max_theta, h, k, b), 1.0);
    }
}
