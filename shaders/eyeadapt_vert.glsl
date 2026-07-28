layout(location = 0) in vec2 in_Position;

void main(void)
{
    gl_Position = vec4(in_Position.xy, 0.0, 1.0);
}
