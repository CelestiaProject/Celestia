uniform vec3 color;
in float shade;

void main(void)
{
    fragColor = vec4(color, shade);
}
