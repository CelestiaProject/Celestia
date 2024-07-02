const float degree_per_px = 0.01;
// empirical constants
const float a = 0.123;
const float k = 0.0016;
const float exposure = 1.0;

// py: max_square_size = 512 # px
const float max_square_size = 256.0;
// py: max_br = (degree_per_px * max_square_size / algorithms.a)**2 / (2*np.pi)
const float max_br = pow((degree_per_px * max_square_size / a), 2.0) / (2.0 * 3.141592653);

varying vec3 v_color;
varying float max_theta;
varying float pointSize;
varying float br;

attribute vec4 in_Position;
attribute vec3 in_Color;
attribute float in_PointSize;

const float color_saturation_limit = 0.1; // the ratio of the minimum color component to the maximum

//! Normalizes the color by its green value and corrects extreme saturation
vec3 green_normalization(vec3 color)
{
    // color /= max(color.r, max(color.g, color.b)); // we do this in XYZRGBConverter::convertUnnormalized()
    float delta = color_saturation_limit - min(color.r, min(color.g, color.b));

    if (delta > 0)
    {
        vec3 diff = vec3(1.0) - color;
        color += diff * diff * delta; // desaturating to the saturation limit
    }
    return color / color.g;
}

void main(void)
{
    // py: linear_br = 10**(-0.4 * star_mag) * exposure # scaled brightness measured in Vegas
    // +0.4 because `in_PointSize` is not the actual magnitude but `faintest` - `actual`
    float br0 = pow(10.0, 0.4 * in_PointSize) * exposure;
    vec3 color = green_normalization(in_Color);
    vec3 scaled_color = color * br0;

    // py: if np.all(scaled_color < 1):
    if (all(lessThan(scaled_color, vec3(1.0)))) // not works! never "true"
    //if (max(scaled_color.r, max(scaled_color.g, scaled_color.b)) < 1.0)
    //if (true)
    {
        // Dim light source (9 pixels mode)
        max_theta = -1.0; // mode indicator
        pointSize = 3.0;
        v_color = scaled_color;
    }
    else
    {
        // Bright light source (glow mode)
        br = atan(br0 / max_br) * max_br; // dimmed brightness
        max_theta = a * sqrt(br); // glow radius
        float half_sq = max_theta / degree_per_px;
        pointSize = 2.0 * half_sq - 1.0;
        v_color = color;
    }

    gl_PointSize = pointSize;
    set_vp(in_Position);
}
