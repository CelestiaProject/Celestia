// packdb.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SPECTRAL_O 0
#define SPECTRAL_B 1
#define SPECTRAL_A 2
#define SPECTRAL_F 3
#define SPECTRAL_G 4
#define SPECTRAL_K 5
#define SPECTRAL_M 6
#define SPECTRAL_R 7
#define SPECTRAL_S 8
#define SPECTRAL_N 9
#define SPECTRAL_WC 10
#define SPECTRAL_WN 11

// Stellar remnants
#define SPECTRAL_WHITE_DWARF  16
#define SPECTRAL_NEUTRON_STAR 32

#define SPECTRAL_UNKNOWN  255

#define LUM_Ia0    0
#define LUM_Ia     1
#define LUM_Ib     2
#define LUM_II     3
#define LUM_III    4
#define LUM_IV     5
#define LUM_V      6
#define LUM_VI     7

#define ID_NONE -1

#define HD_CATALOG          0x00000000
#define HIPPARCOS_CATALOG   0x10000000

#define BINWRITE(fp, n) fwrite(&(n), sizeof(n), 1, (fp))


typedef struct {
    char colorType;
    char subType;
    char luminosity;
    float colorIndex;
} SpectralType;

typedef struct {
    int HIP;          /* HIPPARCOS catalogue number */
    int HD;           /* HD catalogue number */
    float appMag;     /* Apparent magnitude  */
    float RA;         /* 0 -- 24 hours       */
    float dec;        /* -90 -- +90 degrees  */
    float parallax;   /* in milliarcseconds  */
    char spectral[13];
    unsigned char parallaxError;
} Star;

typedef struct {
    int HD;
    char *commonName;
    char *altName;
} HDNameEnt;

/* Hardcoded file names */
#define HIPPARCOS_MAIN_DB "hip_main.dat"
#define COMMON_NAMES_DB   "hdnames.dat"

HDNameEnt *hdNames;
int nHDNames;
Star *Stars;
int nStars;


int CompareHDNameEnt(const void *a, const void *b)
{
    int hda = ((HDNameEnt *) a)->HD;
    int hdb = ((HDNameEnt *) b)->HD;

    if (hda <  hdb)
	return -1;
    else if (hda > hdb)
	return 1;
    else
	return 0;
}


unsigned short PackSpectralType(char *spectralType)
{
    unsigned short packed = 0;
    unsigned short letter;
    unsigned short number;
    unsigned short luminosity = LUM_V;
    int i = 0;

    // Subdwarves (luminosity class VI) are prefixed with sd
    if (spectralType[i] == 's' && spectralType[i + 1] == 'd')
    {
        luminosity = LUM_VI;
        i += 2;
    }

    switch (spectralType[i])
    {
      case 'O':
        letter = SPECTRAL_O;
        break;
      case 'B':
        letter = SPECTRAL_B;
        break;
      case 'A':
        letter = SPECTRAL_A;
        break;
      case 'F':
        letter = SPECTRAL_F;
        break;
      case 'G':
        letter = SPECTRAL_G;
        break;
      case 'K':
        letter = SPECTRAL_K;
        break;
      case 'M':      
        letter = SPECTRAL_M;
        break;
      case 'R':
        letter = SPECTRAL_R;
        break;
      case 'N':
        letter = SPECTRAL_N;
        break;
      case 'S':
        letter = SPECTRAL_S;
        break;
      case 'W':
	i++;
	if (spectralType[i] == 'C')
 	    letter = SPECTRAL_WC;
	else if (spectralType[i] == 'N')
	    letter = SPECTRAL_WN;
	else
	    i--;
	break;
      case 'D':
        letter = SPECTRAL_WHITE_DWARF;
	break;
      default:
        letter = SPECTRAL_UNKNOWN;
        break;
    }

    if (letter == SPECTRAL_WHITE_DWARF)
        return letter << 8;

    i++;
    if (spectralType[i] >= '0' && spectralType[i] <= '9')
        number = spectralType[i] - '0';
    else
        number = 0;

    if (luminosity != LUM_VI)
    {
        i++;
        luminosity = LUM_V;
        while (i < 13 && spectralType[i] != '\0') {
            if (spectralType[i] == 'I') {
                if (spectralType[i + 1] == 'I') {
                    if (spectralType[i + 2] == 'I') {
                        luminosity = LUM_III;
                    } else {
                        luminosity = LUM_II;
                    }
                } else if (spectralType[i + 1] == 'V') {
                    luminosity = LUM_IV;
                } else if (spectralType[i + 1] == 'a') {
                    if (spectralType[i + 2] == '0')
                        luminosity = LUM_Ia0;
                    else
                        luminosity = LUM_Ia;
                } else if (spectralType[i + 1] == 'b') {
                    luminosity = LUM_Ib;
                } else {
                    luminosity = LUM_Ib;
                }
                break;
            } else if (spectralType[i] == 'V') {
                if (spectralType[i + 1] == 'I')
                    luminosity = LUM_VI;
                else
                    luminosity = LUM_V;
                break;
            }
            i++;
        }
    }

    return (letter << 8) | (number << 4) | luminosity;
}


HDNameEnt *ReadCommonNames(FILE *fp)
{
    char buf[256];
    int i;

    hdNames = (HDNameEnt *) malloc(sizeof(HDNameEnt) * 3000);
    if (hdNames == NULL)
	return NULL;

    for (i = 0; ; i++) {
	char *name1, *name2, len;
	if (fgets(buf, 256, fp) == NULL)
	    break;

	/* Strip trailing newline */
	len = strlen(buf);
	if (len > 0 && buf[len - 1] == '\n')
	    buf[len - 1] = '\0';

	name1 = index(buf, ':');
	if (name1 == NULL)
	    break;
	name1[0] = '\0';
	name1++;
	name2 = index(name1, ':');
	if (name2 == NULL)
	    break;
	name2[0] = '\0';
	name2++;
	
	{
	    int hd;
	    if (sscanf(buf, "%d", &hd) != 1)
		break;
	    hdNames[i].HD = hd;
	    if (name1[0] != '\0') {
	        hdNames[i].commonName = (char *) malloc(strlen(name1) + 1);
		strcpy(hdNames[i].commonName, name1);
	    } else if (name2[0] != '\0') {
		hdNames[i].commonName = (char *) malloc(strlen(name2) + 1);
		strcpy(hdNames[i].commonName, name2);
	    }		
	}
    }

    nHDNames = i;
    qsort(hdNames, nHDNames, sizeof(HDNameEnt), CompareHDNameEnt);

    return hdNames;
}


char *LookupName(int HD)
{
    HDNameEnt key;
    HDNameEnt *found;

    key.HD = HD;
    found = (HDNameEnt *) bsearch((void *) &key, (void *) hdNames,
				  nHDNames, sizeof(HDNameEnt),
				  CompareHDNameEnt);
    if (found == NULL)
	return NULL;
    else
	return found->commonName;
}


void WriteStar(FILE *fp, Star *star)
{
    unsigned short spectralType = PackSpectralType(star->spectral);
    short appMag = (short) (star->appMag * 256);
    unsigned int catalog_no;

    if (star->HD != ID_NONE)
        catalog_no = star->HD | HD_CATALOG;
    else
        catalog_no = star->HIP | HIPPARCOS_CATALOG;
    BINWRITE(fp, catalog_no);
    BINWRITE(fp, star->RA);
    BINWRITE(fp, star->dec);
    BINWRITE(fp, star->parallax);
    BINWRITE(fp, appMag);
    BINWRITE(fp, spectralType);
    BINWRITE(fp, star->parallaxError);
}

#define HIPPARCOS_RECORD_LENGTH 512

Star *ReadHipparcosCatalog(FILE *fp)
{
    char buf[HIPPARCOS_RECORD_LENGTH];
    int i;
    int maxStars = 120000;
    Star *stars;
    int nBright = 0, nGood = 0;
    char nameBuf[20];

    fprintf(stderr, "Attempting to allocate %d bytes\n", maxStars * sizeof(Star));
    stars = (Star *) malloc(maxStars * sizeof(Star));
    if (stars == NULL) {
        fprintf(stderr, "Unable to allocate memory for stars.\n");
	return NULL;
    }

    for (i = 0; ; i++) {
	int hh, mm, deg;
	float seconds;
	float parallaxError;
	char degSign;
	
	if (fgets(buf, HIPPARCOS_RECORD_LENGTH, fp) == NULL)
	    break;
	sscanf(buf + 2, "%d", &stars[i].HIP);
	if (sscanf(buf + 390, "%d", &stars[i].HD) != 1)
	    stars[i].HD = ID_NONE;
	sscanf(buf + 41, "%f", &stars[i].appMag);
	sscanf(buf + 79, "%f", &stars[i].parallax);
	sscanf(buf + 17, "%d %d %f", &hh, &mm, &seconds);
	stars[i].RA = hh + (float) mm / 60.0f + (float) seconds / 3600.0f;
	sscanf(buf + 29, "%c%d %d %f", &degSign, &deg, &mm, &seconds);
	stars[i].dec = deg + (float) mm / 60.0f + (float) seconds / 3600.0f;
	if (degSign == '-')
	    stars[i].dec = -stars[i].dec;
	sscanf(buf + 435, "%12s", &stars[i].spectral);
	sscanf(buf + 119, "%f", &parallaxError);
	if (stars[i].parallax <= 0 || parallaxError / stars[i].parallax > 1)
        {
	    stars[i].parallaxError = (unsigned char) 255;
	}
	else
        {
	    stars[i].parallaxError =
		(unsigned char) (parallaxError / stars[i].parallax * 200);
	}

	if (/* stars[i].appMag < 4.0f */
            stars[i].parallax > 0 && 3260 / stars[i].parallax < 20) {
	    nBright++;
#if 0
	    if (parallaxError / stars[i].parallax > 0.25f ||
		parallaxError / stars[i].parallax < 0.0f) {
#endif
	    if (0) {
		char *name = LookupName(stars[i].HD);

		if (name == NULL) {
		    if (stars[i].HD != ID_NONE) {
			sprintf(nameBuf, "HD%d", stars[i].HD);
			name = nameBuf;
		    } else {
			sprintf(nameBuf, "HIP%d", stars[i].HIP);
			name = nameBuf;
		    }
		}
		printf("%-20s %5.2f %6.2f %3d%% %12s %5.2f %5.2f\n",
		       name,
		       stars[i].appMag,
		       3260.0f / stars[i].parallax,
		       (int) (100.0f * parallaxError / stars[i].parallax),
		       stars[i].spectral,
		       stars[i].RA, stars[i].dec);
	    } else {
		nGood++;
	    }
	}
    }

    nStars = i;
    printf("Stars: %d, Bright: %d, Good: %d\n", nStars, nBright, nGood);

    return stars;
}


int main(int argc, char *argv[])
{
    FILE *fp;

    fp = fopen(COMMON_NAMES_DB, "r");
    if (fp == NULL) {
	fprintf(stderr, "Error opening %s\n", COMMON_NAMES_DB);
	return 1;
    }
    hdNames = ReadCommonNames(fp);
    fclose(fp);
    fp = NULL;
    if (hdNames == NULL) {
	fprintf(stderr, "Error reading names file.\n");
	return 1;
    }

    fp = fopen(HIPPARCOS_MAIN_DB, "r");
    if (fp == NULL) {
	fprintf(stderr, "Error opening %s\n", HIPPARCOS_MAIN_DB);
	return 1;
    }
    Stars = ReadHipparcosCatalog(fp);
    fclose(fp);
    if (Stars == NULL) {
	fprintf(stderr, "Error reading HIPPARCOS database.");
	return 1;
    }
#if 0
    {
	int i;

	for (i = 0; i < nStars; i++) {
	    if (Stars[i].spectral[0] == 'O') {
		char *name = LookupName(Stars[i].HD);
		if (name != NULL)
		    printf("%s ", name);
		printf("%6d %6.3f %6.3f %s\n",
		       Stars[i].HD, Stars[i].RA, Stars[i].dec, Stars[i].spectral);
	    }
	}
    }
#endif

    {
        int i;

        FILE *fp = fopen("out", "wb");
        if (fp == NULL) {
	   fprintf(stderr, "Error opening output file.\n");
           exit(1);
        }

	BINWRITE(fp, nStars);
        for (i = 0; i < nStars; i++)
           WriteStar(fp, &Stars[i]);
        
        fclose(fp);
    }
    printf("Stars in catalog = %d\n", nStars);
}
