/* Copyright 2006, Matt Wronkiewicz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the distribution.
 *     * The name of Matt Wronkiewicz may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

struct XYZs {
	float X;
	float Y;
	float Z;
};

struct spectrum {
    float intensity;
    int samples;
};
static struct spectrum spectrum[95];

static float solar_spectrum[95];

static struct XYZs cmfXYZ[] = {
	/*360*/ {0.000000122200f, 0.000000013398f, 0.000000535027f},
	/*365*/ {0.000000919270f, 0.000000100650f, 0.000004028300f},
	/*370*/ {0.000005958600f, 0.000000651100f, 0.000026143700f},
	/*375*/ {0.000033266000f, 0.000003625000f, 0.000146220000f},
	/*380*/ {0.000159952000f, 0.000017364000f, 0.000704776000f},
	/*385*/ {0.000662440000f, 0.000071560000f, 0.002927800000f},
	/*390*/ {0.002361600000f, 0.000253400000f, 0.010482200000f},
	/*395*/ {0.007242300000f, 0.000768500000f, 0.032344000000f},
	/*400*/ {0.019109700000f, 0.002004400000f, 0.086010900000f},
	/*405*/ {0.043400000000f, 0.004509000000f, 0.197120000000f},
	/*410*/ {0.084736000000f, 0.008756000000f, 0.389366000000f},
	/*415*/ {0.140638000000f, 0.014456000000f, 0.656760000000f},
	/*420*/ {0.204492000000f, 0.021391000000f, 0.972542000000f},
	/*425*/ {0.264737000000f, 0.029497000000f, 1.282500000000f},
	/*430*/ {0.314679000000f, 0.038676000000f, 1.553480000000f},
	/*435*/ {0.357719000000f, 0.049602000000f, 1.798500000000f},
	/*440*/ {0.383734000000f, 0.062077000000f, 1.967280000000f},
	/*445*/ {0.386726000000f, 0.074704000000f, 2.027300000000f},
	/*450*/ {0.370702000000f, 0.089456000000f, 1.994800000000f},
	/*455*/ {0.342957000000f, 0.106256000000f, 1.900700000000f},
	/*460*/ {0.302273000000f, 0.128201000000f, 1.745370000000f},
	/*465*/ {0.254085000000f, 0.152761000000f, 1.554900000000f},
	/*470*/ {0.195618000000f, 0.185190000000f, 1.317560000000f},
	/*475*/ {0.132349000000f, 0.219940000000f, 1.030200000000f},
	/*480*/ {0.080507000000f, 0.253589000000f, 0.772125000000f},
	/*485*/ {0.041072000000f, 0.297665000000f, 0.570600000000f},
	/*490*/ {0.016172000000f, 0.339133000000f, 0.415254000000f},
	/*495*/ {0.005132000000f, 0.395379000000f, 0.302356000000f},
	/*500*/ {0.003816000000f, 0.460777000000f, 0.218502000000f},
	/*505*/ {0.015444000000f, 0.531360000000f, 0.159249000000f},
	/*510*/ {0.037465000000f, 0.606741000000f, 0.112044000000f},
	/*515*/ {0.071358000000f, 0.685660000000f, 0.082248000000f},
	/*520*/ {0.117749000000f, 0.761757000000f, 0.060709000000f},
	/*525*/ {0.172953000000f, 0.823330000000f, 0.043050000000f},
	/*530*/ {0.236491000000f, 0.875211000000f, 0.030451000000f},
	/*535*/ {0.304213000000f, 0.923810000000f, 0.020584000000f},
	/*540*/ {0.376772000000f, 0.961988000000f, 0.013676000000f},
	/*545*/ {0.451584000000f, 0.982200000000f, 0.007918000000f},
	/*550*/ {0.529826000000f, 0.991761000000f, 0.003988000000f},
	/*555*/ {0.616053000000f, 0.999110000000f, 0.001091000000f},
	/*560*/ {0.705224000000f, 0.997340000000f, 0.000000000000f},
	/*565*/ {0.793832000000f, 0.982380000000f, 0.000000000000f},
	/*570*/ {0.878655000000f, 0.955552000000f, 0.000000000000f},
	/*575*/ {0.951162000000f, 0.915175000000f, 0.000000000000f},
	/*580*/ {1.014160000000f, 0.868934000000f, 0.000000000000f},
	/*585*/ {1.074300000000f, 0.825623000000f, 0.000000000000f},
	/*590*/ {1.118520000000f, 0.777405000000f, 0.000000000000f},
	/*595*/ {1.134300000000f, 0.720353000000f, 0.000000000000f},
	/*600*/ {1.123990000000f, 0.658341000000f, 0.000000000000f},
	/*605*/ {1.089100000000f, 0.593878000000f, 0.000000000000f},
	/*610*/ {1.030480000000f, 0.527963000000f, 0.000000000000f},
	/*615*/ {0.950740000000f, 0.461834000000f, 0.000000000000f},
	/*620*/ {0.856297000000f, 0.398057000000f, 0.000000000000f},
	/*625*/ {0.754930000000f, 0.339554000000f, 0.000000000000f},
	/*630*/ {0.647467000000f, 0.283493000000f, 0.000000000000f},
	/*635*/ {0.535110000000f, 0.228254000000f, 0.000000000000f},
	/*640*/ {0.431567000000f, 0.179828000000f, 0.000000000000f},
	/*645*/ {0.343690000000f, 0.140211000000f, 0.000000000000f},
	/*650*/ {0.268329000000f, 0.107633000000f, 0.000000000000f},
	/*655*/ {0.204300000000f, 0.081187000000f, 0.000000000000f},
	/*660*/ {0.152568000000f, 0.060281000000f, 0.000000000000f},
	/*665*/ {0.112210000000f, 0.044096000000f, 0.000000000000f},
	/*670*/ {0.081260600000f, 0.031800400000f, 0.000000000000f},
	/*675*/ {0.057930000000f, 0.022601700000f, 0.000000000000f},
	/*680*/ {0.040850800000f, 0.015905100000f, 0.000000000000f},
	/*685*/ {0.028623000000f, 0.011130300000f, 0.000000000000f},
	/*690*/ {0.019941300000f, 0.007748800000f, 0.000000000000f},
	/*695*/ {0.013842000000f, 0.005375100000f, 0.000000000000f},
	/*700*/ {0.009576880000f, 0.003717740000f, 0.000000000000f},
	/*705*/ {0.006605200000f, 0.002564560000f, 0.000000000000f},
	/*710*/ {0.004552630000f, 0.001768470000f, 0.000000000000f},
	/*715*/ {0.003144700000f, 0.001222390000f, 0.000000000000f},
	/*720*/ {0.002174960000f, 0.000846190000f, 0.000000000000f},
	/*725*/ {0.001505700000f, 0.000586440000f, 0.000000000000f},
	/*730*/ {0.001044760000f, 0.000407410000f, 0.000000000000f},
	/*735*/ {0.000727450000f, 0.000284041000f, 0.000000000000f},
	/*740*/ {0.000508258000f, 0.000198730000f, 0.000000000000f},
	/*745*/ {0.000356380000f, 0.000139550000f, 0.000000000000f},
	/*750*/ {0.000250969000f, 0.000098428000f, 0.000000000000f},
	/*755*/ {0.000177730000f, 0.000069819000f, 0.000000000000f},
	/*760*/ {0.000126390000f, 0.000049737000f, 0.000000000000f},
	/*765*/ {0.000090151000f, 0.000035540500f, 0.000000000000f},
	/*770*/ {0.000064525800f, 0.000025486000f, 0.000000000000f},
	/*775*/ {0.000046339000f, 0.000018338400f, 0.000000000000f},
	/*780*/ {0.000033411700f, 0.000013249000f, 0.000000000000f},
	/*785*/ {0.000024209000f, 0.000009619600f, 0.000000000000f},
	/*790*/ {0.000017611500f, 0.000007012800f, 0.000000000000f},
	/*795*/ {0.000012855000f, 0.000005129800f, 0.000000000000f},
	/*800*/ {0.000009413630f, 0.000003764730f, 0.000000000000f},
	/*805*/ {0.000006913000f, 0.000002770810f, 0.000000000000f},
	/*810*/ {0.000005093470f, 0.000002046130f, 0.000000000000f},
	/*815*/ {0.000003767100f, 0.000001516770f, 0.000000000000f},
	/*820*/ {0.000002795310f, 0.000001128090f, 0.000000000000f},
	/*825*/ {0.000002082000f, 0.000000842160f, 0.000000000000f},
	/*830*/ {0.000001553140f, 0.000000629700f, 0.000000000000f}
};

static float interpolate_linear_f(const float xa, const float ya,
	const float xb, const float yb, const float x) {
    const float interval = xb - xa;
    const float dx = (interval == 0)?0:(x - xa)/interval;
    return ya + (yb - ya)*dx;
}

int main(const int argc, const char* const* const argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: [path to solar spectrum] [path to asteroid spectrum]\n");
        fprintf(stderr, "Solar spectrum must be a list in the format\n"
            "\t'[wavelength in nm] [intensity]'\n");
        fprintf(stderr, "Asteroid spectrum must be a list in the format\n"
            "\t'[wavelength in microns] [reflectiveness]'\n");
        fprintf(stderr, "All list values must be in easily digestable floating point format\n");
        fprintf(stderr, "Data files from sets 2, 7, and 8 of SMASS are in the correct format\n");
        return -1;
    }
    /* Zero out the asteroid spectrum */
    int i = 0;
    for (; i < 95; i++) {
        spectrum[i].samples = 0;
        spectrum[i].intensity = 0;
    }
    FILE* const input = fopen(argv[2], "r");
    int first_wave = 0;
    float first_i = 0;
    while (1) {
        char line[256];
        char* read_ptr = fgets(line, 256, input);
        if (read_ptr == NULL)
            break;
        const float wave_f = strtod(read_ptr, &read_ptr);
        const int wave = wave_f > 10?wave_f:wave_f*10000;
        if (wave == 0)
            continue;
        if (wave < 3600 || wave > 8300)
            continue;
        const float intensity = strtod(read_ptr, NULL);
        spectrum[(wave - 3600 + 25)/50].samples++;
        spectrum[(wave - 3600 + 25)/50].intensity += intensity;
        if (first_wave == 0) {
            first_wave = wave;
            first_i = intensity;
        }
    }
    fclose(input);
    FILE* const sol_input = fopen(argv[1], "r");
    if (sol_input == NULL) {
	fprintf(stderr, "Couldn't open solar spectrum\n");
	return -1;
    }
    while (1) {
        char line[256];
        char* read_ptr = fgets(line, 256, sol_input);
        if (read_ptr == NULL)
            break;
        const int wave = strtod(read_ptr, &read_ptr);
	if (wave == 0)
	    continue;
	if (wave > 830)
	    continue;
        const float intensity = strtod(read_ptr, NULL);
        solar_spectrum[(wave - 360)/5] = intensity;
    }
    fclose(sol_input);
    
    float X = 0;
    float Y = 0;
    float Z = 0;
    int w = 360;
    for (; w <= 830; w += 5) {
        const int tblI = (w - 360)/5;
        float intensity = interpolate_linear_f(0, 0, first_wave/10, first_i, w);
        if (intensity > first_i) { intensity = first_i; }
        if (intensity < 0) intensity = 0;
        if (spectrum[tblI].samples > 0)
            intensity = spectrum[tblI].intensity/spectrum[tblI].samples;
        /* Convert reflective value to emissive */
        intensity *= solar_spectrum[tblI];
        /*printf("%d %.3f\n", w, intensity);*/
        /* The color perception table has been trimmed to 1/5th its original
         * size. Scale the values accordingly. */
        intensity *= 5;
        X += cmfXYZ[tblI].X*intensity;
        Y += cmfXYZ[tblI].Y*intensity;
        Z += cmfXYZ[tblI].Z*intensity;
    }
    /* The color perception table adds up to 100 instead of 1 */
    X /= 100.0f;
    Y /= 100.0f;
    Z /= 100.0f;
    /* Convert the XYZ colors into RGB */
    float r = X*3.24f - Y*1.54f - Z*0.50f;
    float g = -X*0.97f + Y*1.88f + Z*0.04f;
    float b = X*0.06f - Y*0.20f + Z*1.06f;
    
    /* Gamma correction */
    /* I am not sure that this is necessary or correct. It has the effect of de-saturating
     * the asteroid colors. */
    r = 1.055f*pow(r, 1/2.0f) - 0.055f;
    g = 1.055f*pow(g, 1/2.0f) - 0.055f;
    b = 1.055f*pow(b, 1/2.0f) - 0.055f;
    
    /* Scale values to maximize the luminosity */
    float max = r;
    if (g > max) max = g;
    if (b > max) max = b;
    r /= max;
    g /= max;
    b /= max;
    
    printf("\tColor [ %.3f %.3f %.3f ]\n", r, g, b);
    return 0;
}

