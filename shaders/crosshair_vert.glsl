attribute vec4 in_Position;

uniform float radius;
uniform float width;
uniform float h;
uniform float angle;

void main(void)
{
    float c = cos(angle);
    float s = sin(angle);
    mat3 rot = mat3(c, s, 0.0, -s, c, 0.0, 0.0, 0.0, 1.0);

    float x = in_Position.x * width + radius;
    float y = in_Position.y * h;
    vec3 p = rot * vec3(x, y, 0.0);
    gl_Position = MVPMatrix * vec4(p, 1.0);
}
