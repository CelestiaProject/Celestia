
const float degree_per_px = 0.05;
const float br_limit = 1.0 / (255.0 * 12.92);

varying vec3 v_color; // 12
varying vec3 v_tetha_k; // 24
varying float pointSize; // 28

uniform vec2 viewportSize;

attribute vec4 in_Position;
attribute vec3 in_Color;
attribute float in_PointSize; // scaled brightness measured in Vegas


void main(void)
{
    float linearBr = pow(10.0, 0.4f * in_PointSize) * br_limit;
    vec3 color0 = in_Color * (linearBr / in_Color.g); // scaling on brightness and normalizing by green channel

    vec3 check_vec = step(vec3(1.0), color0); // step(edge, x) - For element i of the return value, 0.0 is returned if x[i] < edge[i], and 1.0 is returned otherwise.
    float check = check_vec.x + check_vec.y + check_vec.z;
    if (check == 0.0)
    {
        pointSize = 1.0;
        v_tetha_k = vec3(-1.0);
    }
    else
    {
        float max_br = sqrt(max(color0.r, max(color0.g, color0.b)));
        float max_theta = 0.2 * sqrt(max_br); // glare radius
        float k = 3.3e-5 * pow(max_theta, -2.5); // common constant, depending originally on star brightness
        float min_theta = max_theta / (pow(k, -0.5) + 1.0);
        pointSize = floor(max_theta / (sqrt(0.5 * br_limit / (k * max_br)) + 1.0) / degree_per_px);
        v_tetha_k = vec3(max_theta, min_theta, k);
    }

    gl_PointSize = pointSize;
    v_color = color0;
    set_vp(in_Position);
}
