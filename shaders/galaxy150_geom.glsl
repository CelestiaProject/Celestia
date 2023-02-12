
uniform mat4 m;
uniform mat3 viewMat;

uniform float size;
uniform float brightness;
uniform float minimumFeatureSize;

in Vertex
{
    vec3  color;
    float size;
    float brightness;
} vertex[];

out vec4 v_Color;
out vec2 v_TexCoord;

void main()
{
    float s = size * vertex[0].size;
    if (s >= minimumFeatureSize)
    {
        vec4 p = m * gl_in[0].gl_Position;
        float screenFrac = s / length(p);
        if (screenFrac < 0.1)
        {
            /*
             * This shader assumes that vertices are rendered in CCW order.
             */
            vec4 v0 = vec4(viewMat * vec3(-1.0,  1.0, 0.0) * s, 0.0);
            vec4 v1 = vec4(viewMat * vec3(-1.0, -1.0, 0.0) * s, 0.0);
            vec4 v2 = vec4(viewMat * vec3( 1.0,  1.0, 0.0) * s, 0.0);
            vec4 v3 = vec4(viewMat * vec3( 1.0, -1.0, 0.0) * s, 0.0);
            float alpha = (0.1 - screenFrac) * vertex[0].brightness * brightness;
            vec4 color = vec4(vertex[0].color, alpha);

            gl_Position = MVPMatrix * (p + v0);
            v_TexCoord  = vec2(0.0, 1.0);
            v_Color     = color;
            EmitVertex();

            gl_Position = MVPMatrix * (p + v1);
            v_TexCoord  = vec2(0.0, 0.0);
            v_Color     = color;
            EmitVertex();

            gl_Position = MVPMatrix * (p + v2);
            v_TexCoord  = vec2(1.0, 1.0);
            v_Color     = color;
            EmitVertex();

            gl_Position = MVPMatrix * (p + v3);
            v_TexCoord  = vec2(1.0, 0.0);
            v_Color     = color;
            EmitVertex();
        }
    }
    EndPrimitive();
}
