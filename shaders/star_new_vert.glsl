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

uniform float magScale;
/*uniform*/ const float sigma2 = 0.35;
/*uniform highp*/ const float glareFalloff = 1.0 / 15.0;
/*uniform highp*/ const float glareBrightness = 0.003;
uniform float exposure;
uniform float thresholdBrightness;

varying vec2 pointCenter;
varying vec4 color;
varying float brightness;


void main()
{
    vec4 position = vec4(in_Position, 1.0);
    float appMag = in_PointSize;

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
