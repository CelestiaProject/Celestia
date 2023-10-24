
const float degree_per_px = 0.05;
const float br_limit = 1.0 / (255.0 * 12.92);

varying vec3 v_color; // 12
varying vec3 v_max_tetha_hk; // 24
varying float pointSize; // 28

uniform vec2 viewportSize;

attribute vec4 in_Position;
attribute vec3 in_Color;
attribute float in_PointSize; // scaled brightness measured in Vegas

float psf_max_theta(float br)
{
    return 0.012 + 1.488 / (sqrt(br_limit * 1000000.0 / (11.0 * br)) + 1.0);
}

void main(void)
{
    float linearBr = pow(10.0, 0.4f * in_PointSize) * br_limit;
    vec3 color0 = in_Color * (linearBr / in_Color.g); // scaling on brightness and normalizing by green channel

    vec3 check_vec = step(vec3(1.0), color0); // step(edge, x) - For element i of the return value, 0.0 is returned if x[i] < edge[i], and 1.0 is returned otherwise.
    float check = check_vec.x + check_vec.y + check_vec.z;
    if (check == 0.0)
    {
        pointSize = 1.0;
        v_max_tetha_hk = vec3(-1.0);
    }
    else
    {
        float max_theta = 0.33435822702992773 * sqrt(max(color0.r, max(color0.g, color0.b))); // glare radius
        pointSize =  2.0 * ceil(max_theta / degree_per_px);

        float h = 0.0082234880783653 * pow(max_theta, 0.7369983254906639); // h, k, b - common constants, depending originally on star brightness
        float k = 38581.577272697796 * pow(max_theta, 2.368787717957141);
        v_max_tetha_hk = vec3(max_theta, h, k);
    }

    gl_PointSize = pointSize;
    v_color = color0;
    set_vp(in_Position);
}
