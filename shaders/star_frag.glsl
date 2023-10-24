const float degree_per_px = 0.05;
const float br_limit = 1.0 / (255.0 * 12.92);

varying vec3 v_color;
varying vec3 v_tetha_k;
varying float pointSize;

float psf_square(float theta, float min_theta, float max_theta, float k)
{
    // Human eye's point source function, optimized to fit a square.
    // Lower limit on brightness and angular size: 1 Vega and 0.05 degrees per pixel.
    // No upper limits.

    if (theta < min_theta)
        return 1.0; // overexposed

    if (theta < max_theta)
    {
        float brackets = max_theta / theta - 1.0;
        return k * brackets * brackets;
    }

    return 0.0;
}

void main(void)
{
    float max_theta = v_tetha_k.x;
    if (max_theta == -1.0)
    {
        gl_FragColor = vec4(v_color, 1.0);
    }
    else
    {
        float min_theta = v_tetha_k.y;
        float k = v_tetha_k.z;
        // Option 2: glare square render
        vec2 offset = (gl_PointCoord.xy - vec2(0.5)) * pointSize;
        float theta = length(offset) * degree_per_px;    

        gl_FragColor = vec4(v_color * psf_square(theta, min_theta, max_theta, k), 1.0);
    }
}
