// HDR frame texture with a full mipmap chain; top level holds the frame average.
uniform sampler2D hdrTex;

// Previous frame's adapted exposure factor (1x1).
uniform sampler2D prevAdapt;

uniform float topLod;      // mip level of the 1x1 frame average
uniform float dt;          // seconds since last adaptation
uniform float keyValue;    // target average scene luminance
uniform float minExposure;
uniform float maxExposure; // <= 1.0 keeps auto-exposure darken-only
uniform float adaptTau;    // eye-adaptation time constant (s)

void main(void)
{
    vec3 avg = textureLod(hdrTex, vec2(0.5), topLod).rgb;
    float avgLuma = dot(avg, vec3(0.2126, 0.7152, 0.0722));
    float target = clamp(keyValue / max(avgLuma, 1.0e-6), minExposure, maxExposure);

    float prev = texture(prevAdapt, vec2(0.5)).r;
    float adapted;
    if (prev <= 0.0 || dt <= 0.0)
    {
        adapted = target;
    }
    else
    {
        float blend = 1.0 - exp(-dt / adaptTau);
        float logCur = log(max(prev, minExposure));
        adapted = exp(logCur + (log(target) - logCur) * blend);
    }

    fragColor = vec4(adapted, adapted, adapted, 1.0);
}
