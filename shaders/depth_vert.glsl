attribute vec4 in_Position;

void main(void)
{
    gl_Position = MVPMatrix * in_Position;
}
