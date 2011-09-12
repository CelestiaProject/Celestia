// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// vsoptruct-rect.c
// Truncate VSOP87 series for rectangular variables to a specified error and
// generate C source code

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define LINE_LENGTH 134

// max error in AU
static double maxError[6] = {
    1e-6, 5e-7, 1e-7, 5e-8, 1e-8, 5e-9
};


// Command line args:
//    vsoptrunc-rect <body name> <error multiplier>
int main(int argc, char* argv[])
{
    char buf[512];
    int degree = 0;
    int xyz = 0;
    const char* planet = "earth";
    int term = 0;
    int truncSeries = 0;
    double a0 = 1.0;

    if (argc > 1)
        planet = argv[1];
    if (argc > 2)
        a0 = atof(argv[2]);

    while (fgets(buf, LINE_LENGTH + 1, stdin))
    {
        if (!strncmp(buf, " VSOP87", 7))
        {
            int d = (int) buf[59] - (int) '0';
            if (d < 0 || d > 5)
            {
                fprintf(stderr, "Bad degree in VSOP data file\n");
                exit(1);
            }

            if (d < degree)
                xyz++;
            if (xyz > 2)
            {
                fprintf(stderr, "More than three variables in VSOP file?\n");
                exit(1);
            }

            degree = d;

            if (degree != 0 || xyz != 0)
                printf("};\n\n");
            printf("static VSOPTerm %s_%c%d[] = {\n",
                   planet, "XYZ"[xyz], degree);
            term = 0;
            truncSeries = 0;
                   
        }
        else
        {
            double a, b, c;
            double maxerr;

            if (sscanf(buf + 80, " %lf %lf %lf", &a, &b, &c) != 3)
            {
                fprintf(stderr, "Bad numbers in VSOP file\n");
                exit(1);
            }

            term++;
            maxerr = 2 * sqrt(term) * a;
            if (!truncSeries)
            {
                if (maxerr < maxError[degree] * a0)
                {
                    truncSeries = 1;
                    if (term == 1)
                        printf("    { 0, 0, 0 },\n");
                    printf("    // %d terms retained\n", term - 1);
                }
            }

            // printf("%.12lf   ", maxerr);

            if (!truncSeries)
                printf("    { %.12g, %.12g, %.12g },\n", a, b, c);
        }
    }

    printf("};\n\n");

    return 0;
}
