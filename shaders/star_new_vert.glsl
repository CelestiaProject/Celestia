#ifndef GL_ES
#define highp
#define mediump
#define lowp
#endif

attribute vec3 in_Position;
attribute float in_PointSize;
attribute vec4 in_Color;

uniform vec2 viewportSize;
uniform vec2 viewportCoord;

uniform /*const*/ float magScale/* = 1.3369011415551906*/;
/*uniform*/ const float sigma2 = 0.35;
/*uniform highp*/ const float glareFalloff = 1.0 / 15.0;
/*uniform highp*/ const float glareBrightness = 0.003;
uniform /*const*/ float exposure /*= 21.72534925735687|74.430977*/;
uniform /*const*/ float thresholdBrightness/* = 1.0 / 255.0*/;

varying vec2 pointCenter;
varying vec4 color;
varying float brightness;


void main()
{
    vec4 position = vec4(in_Position, 1.0);
    float appMag = /*position.z*/in_PointSize;
    /*position.z = sqrt(1.0 - dot(position.xy, position.xy)) * sign(in_Color.a - 0.5);*/

    vec4 projectedPosition = MVPMatrix * position;
    vec2 devicePosition = projectedPosition.xy / projectedPosition.w;
    pointCenter = (devicePosition * 0.5 + vec2(0.5, 0.5)) * viewportSize + viewportCoord;
    color = in_Color;
    float b = pow(2.512, -appMag * magScale);
    float r2 = -log(thresholdBrightness / (exposure * b)) * 2.0 * sigma2;
    float rGlare2 = (exposure * glareBrightness * b / thresholdBrightness - 1.0) / glareFalloff;
    gl_PointSize = 2.0 * sqrt(max(r2, rGlare2));

    brightness = b;
    gl_Position = projectedPosition;
}
