#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ID_NONE -1

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

#define LUM_Ia     0
#define LUM_Ib     1
#define LUM_II     2
#define LUM_III    3
#define LUM_IV     4
#define LUM_V      5

typedef struct {
    char colorType;
    char subType;
    char luminosity;
    float colorIndex;
} SpectralType;

typedef struct {
    int HIP;          /* HIPPARCOS catalogue number */
    int HD;           /* HD catalogue number */
    char *commonName;
    float appMag;     /* Apparent magnitude  */
    float RA;         /* 0 -- 24 hours       */
    float dec;        /* -90 -- +90 degrees  */
    float parallax;   /* in milliarcseconds  */
    char spectral[13];
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

HDNameEnt *ReadCommonNames(FILE *fp)
{
    char buf[256];
    int i;

    hdNames = malloc(sizeof(HDNameEnt) * 3000);
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
		hdNames[i].commonName = malloc(strlen(name1) + 1);
		strcpy(hdNames[i].commonName, name1);
	    } else if (name2[0] != '\0') {
		hdNames[i].commonName = malloc(strlen(name2) + 1);
		strcpy(hdNames[i].commonName, name2);
	    }		
	}
    }

    nHDNames = i;
    qsort(hdNames, nHDNames, sizeof(HDNameEnt), CompareHDNameEnt);
#if 0
    for (i = 0; i < nHDNames; i++) {
	if (hdNames[i].commonName != NULL) {
	    printf("%6d %s\n", hdNames[i].HD, hdNames[i].commonName);
	}
    }
#endif

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


#define HIPPARCOS_RECORD_LENGTH 512

Star *ReadHipparcosCatalog(FILE *fp)
{
    char buf[HIPPARCOS_RECORD_LENGTH];
    int i;
    int maxStars = 120000;
    Star *stars;
    int nBright = 0, nGood = 0;
    char nameBuf[20];

    stars = malloc(maxStars * sizeof(Star));
    if (stars == NULL)
	return NULL;

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
	if (/* stars[i].appMag < 4.0f */
            stars[i].parallax > 0 && 3260 / stars[i].parallax < 20) {
	    nBright++;
#if 0
	    if (parallaxError / stars[i].parallax > 0.25f ||
		parallaxError / stars[i].parallax < 0.0f) {
#endif
	    if (1) {
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

    printf("Stars in catalog = %d\n", nStars);
}




