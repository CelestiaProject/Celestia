attribute vec3 in_Position;
attribute vec4 in_Color;
attribute float starSize;
attribute float eta;

uniform mat3 m;
uniform vec3 offset;
uniform float brightness;
uniform float pixelWeight;
uniform float RRatio;

const float RRatio_min   = pow(10.0, 1.7);
const float clipDistance = 100.0; // observer distance [ly] from globular, where we
                                  // start "morphing" the star-sprite sizes towards
                                  // their physical values

varying vec4 color;

float relStarDensity(void)
{
    /*! As alpha blending weight (relStarDensity) I take the theoretical
     *  number of globular stars in 2d projection at a distance
     *  rho = r / r_c = eta * r_t from the center (cf. King_1962's Eq.(18)),
     *  divided by the area = PI * rho * rho . This number density of stars
     *  I normalized to 1 at rho=0.

     *  The resulting blending weight increases strongly -> 1 if the
     *  2d number density of stars rises, i.e for rho -> 0.
     */

     float rRatio = max(RRatio_min, RRatio);
     float Xi = 1.0 / sqrt(1.0 + rRatio * rRatio);
     float XI2 = Xi * Xi;
     float rho2 = 1.0001 + eta * eta * rRatio * rRatio; //add 1e-4 as regulator near rho=0

     return ((log(rho2) + 4.0 * (1.0 - sqrt(rho2)) * Xi) / (rho2 - 1.0) + XI2) / (1.0 - 2.0 * Xi + XI2);
}


void main(void)
{
    vec3 p = m * in_Position.xyz;
    float br = 2.0 * brightness;

    vec4 mod = ModelViewMatrix * vec4(in_Position, 1.0);
    float s = 2000.0 / -mod.z * br * starSize;

    float obsDistanceToStarRatio = length(p + offset) / clipDistance;
    // "Morph" the star-sprite sizes at close observer distance such that
    // the overdense globular core is dissolved upon closing in.
    gl_PointSize = s * min(obsDistanceToStarRatio, 1.0);

    color = vec4(in_Color.rgb, min(1.0, br * (1.0 - pixelWeight * relStarDensity())));
    gl_Position = MVPMatrix * vec4(p, 1.0);
}
