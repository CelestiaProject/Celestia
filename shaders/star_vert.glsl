
const float degree_per_px = 0.05;
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

const float color_saturation_limit = 0.1; // The ratio of the minimum color component to the maximum

//! Normalizes the color by its green value and corrects extreme saturation
// py: def green_normalization(color: np.ndarray):
vec3 green_normalization(vec3 color)
{
    // py: color /= color.max()
    // color /= max(color.r, max(color.g, color.b)); // we do this in XYZRGBConverter::convertUnnormalized()

    // py: delta = color_saturation_limit - color.min()
    float delta = color_saturation_limit - min(color.r, min(color.g, color.b));

    // py: if delta > 0:
    if (delta > 0)
    {
        // py: color += delta * (1-color)**2 # desaturating to the saturation limit
        vec3 diff = vec3(1.0) - color;
        color += delta * (diff * diff); // desaturating to the saturation limit
    }
    // py: return color / color[1]
    return color / color.g;
}

void main(void)
{
    // py: linear_br = 10**(-0.4 * star_mag) * exposure # scaled brightness measured in Vegas
    // here i use +0.4 because `in_PointSize` is not the actual magnitude but `faintest` - `actual`
    float br0 = pow(10.0, 0.4 * in_PointSize) * exposure;

    // py: color = auxiliary.green_normalization(color0)
    vec3 color = green_normalization(in_Color);

    // py: scaled_color = color * br0
    vec3 scaled_color = color * br0;

    // py: if np.all(scaled_color < 1):
    if (all(lessThan(scaled_color, vec3(1.0))))
    {
        // we set color in the fragment shader (using v_color) so here we just set point size to 1px
        pointSize = 1.0;
        // use max_theta == -1 as as signal that the point size is 1px
        max_theta = -1.0;
    }
    else
    {
        // py: br = np.arctan(br0 / max_br) * max_br # dimmed brightness
        br = atan(br0 / max_br) * max_br; // dimmed brightness
        // py: max_theta = a * np.sqrt(br) # glow radius
        max_theta = a * sqrt(br); // glow radius
        // py: half_sq = floor(max_theta / degree_per_px)
        float half_sq = max_theta / degree_per_px;
        // py: x_min = -min(half_sq, center[0])
        // py: x_max = min(half_sq+1, width-center[0])
        // py: y_min = -min(half_sq, center[1])
        // py: y_max = min(half_sq+1, hight-center[1])
        // py: x = np.arange(x_min, x_max)
        // py: y = np.arange(y_min, y_max)
        // py: xx, yy = np.meshgrid(x, y)
        // we just set a point size. all iteration over every px is done in the fragment shader
        pointSize = 2.0 * half_sq - 1.0;
    }

    gl_PointSize = pointSize;
    v_color = scaled_color;
    set_vp(in_Position);
}
