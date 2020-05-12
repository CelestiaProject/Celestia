attribute vec2 in_Position;

uniform float pixelSize;
uniform float s, c;
uniform float x0, y0;
uniform vec3 u, v;

void main(void)
{
    float x = in_Position.x * pixelSize;
    float y = in_Position.y * pixelSize;
    vec3 pos = (x * c - y * s + x0) * u + (x * s + y * c + y0) * v;
    gl_Position = MVPMatrix * vec4(pos, 1.0);
}
