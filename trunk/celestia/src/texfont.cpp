/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees  and is
   provided without guarantee or warrantee expressed or  implied. This
   program is -not- in the public domain. */

#if defined(_WIN32)
#pragma warning (disable:4244)	/* disable bogus conversion warnings */
#endif

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/glu.h>
#include "texfont.h"

using namespace std;

#if 0
/* Uncomment to debug various scenarios. */
#undef GL_VERSION_1_1
#undef GL_EXT_texture_object
#undef GL_EXT_texture
#endif

#ifndef GL_VERSION_1_1

#  if defined(GL_EXT_texture_object)
#    define glGenTextures glGenTexturesEXT
#    define glBindTexture glBindTextureEXT
#  else
	 /* Without OpenGL 1.1 or the texture object extension, use display lists. */
#    define USE_DISPLAY_LISTS
#  endif

#  if defined(GL_EXT_texture)
#    define GL_INTENSITY4 GL_INTENSITY4_EXT
int useLuminanceAlpha = 0;
#  else
	 /* Intensity texture format not in OpenGL 1.0; added by the EXT_texture
	    extension and now part of OpenGL 1.1. */
int useLuminanceAlpha = 1;
#  endif

#else
   /* OpenGL 1.1 case. */
int useLuminanceAlpha = 0;
#endif

static TexGlyphVertexInfo NullGlyph = {
    { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
    { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
    0
};

/* byte swap a 32-bit value */
#define SWAPL(x, n) { \
                 n = ((char *) (x))[0];\
                 ((char *) (x))[0] = ((char *) (x))[3];\
                 ((char *) (x))[3] = n;\
                 n = ((char *) (x))[1];\
                 ((char *) (x))[1] = ((char *) (x))[2];\
                 ((char *) (x))[2] = n; }

/* byte swap a short */
#define SWAPS(x, n) { \
                 n = ((char *) (x))[0];\
                 ((char *) (x))[0] = ((char *) (x))[1];\
                 ((char *) (x))[1] = n; }

static TexGlyphVertexInfo *getTCVI(TexFont * txf, int c)
{
	TexGlyphVertexInfo *tgvi;

	/* Automatically substitute uppercase letters with lowercase if not
	   uppercase available (and vice versa). */
	if ((c >= txf->min_glyph) && (c < txf->min_glyph + txf->range)) {
		tgvi = txf->lut[c - txf->min_glyph];
		if (tgvi) {
			return tgvi;
		}
		if (islower(c)) {
			c = toupper(c);
			if ((c >= txf->min_glyph) && (c < txf->min_glyph + txf->range)) {
				return txf->lut[c - txf->min_glyph];
			}
		}
		if (isupper(c)) {
			c = tolower(c);
			if ((c >= txf->min_glyph) && (c < txf->min_glyph + txf->range)) {
				return txf->lut[c - txf->min_glyph];
			}
		}
	}

        return &NullGlyph;
#if 0
	fprintf(stderr,
			"texfont: tried to access unavailable font character \"%c\" (%d)\n",
			isprint(c) ? c : ' ', c);
	abort();

	/* NOTREACHED */
#endif
}

static char *lastError;

char *txfErrorString(void)
{
	return lastError;
}

TexFont *txfLoadFont(char *filename)
{
	TexFont *txf;
	FILE *file;
	GLfloat w, h, xstep, ystep;
	char fileid[4], tmp;
	unsigned char *texbitmap;
	int min_glyph, max_glyph;
	int endianness, swap, format, stride, width, height;
	int i, j;
	unsigned long got;

	txf = NULL;
	file = fopen(filename, "rb");
	if (file == NULL) {
		lastError = "file open failed.";
		goto error;
	}
	txf = (TexFont *) malloc(sizeof(TexFont));
	if (txf == NULL) {
		lastError = "out of memory.";
		goto error;
	}
	/* For easy cleanup in error case. */
	txf->tgi = NULL;
	txf->tgvi = NULL;
	txf->lut = NULL;
	txf->teximage = NULL;

	got = fread(fileid, 1, 4, file);
	if (got != 4 || strncmp(fileid, "\377txf", 4)) {
		lastError = "not a texture font file.";
		goto error;
	}
	/*CONSTANTCONDITION*/ assert(sizeof(int) == 4);	/* Ensure external file format size. */
	got = fread(&endianness, sizeof(int), 1, file);
	if (got == 1 && endianness == 0x12345678) {
		swap = 0;
	} else if (got == 1 && endianness == 0x78563412) {
		swap = 1;
	} else {
		lastError = "not a texture font file.";
		goto error;
}
#define EXPECT(n) if (got != (unsigned long) n) { lastError = "premature end of file."; goto error; }
	got = fread(&format, sizeof(int), 1, file);
	EXPECT(1);
	got = fread(&txf->tex_width, sizeof(int), 1, file);
	EXPECT(1);
	got = fread(&txf->tex_height, sizeof(int), 1, file);
	EXPECT(1);
	got = fread(&txf->max_ascent, sizeof(int), 1, file);
	EXPECT(1);
	got = fread(&txf->max_descent, sizeof(int), 1, file);
	EXPECT(1);
	got = fread(&txf->num_glyphs, sizeof(int), 1, file);
	EXPECT(1);

	if (swap) {
		SWAPL(&format, tmp);
		SWAPL(&txf->tex_width, tmp);
		SWAPL(&txf->tex_height, tmp);
		SWAPL(&txf->max_ascent, tmp);
		SWAPL(&txf->max_descent, tmp);
		SWAPL(&txf->num_glyphs, tmp);
	}
	txf->tgi =
		(TexGlyphInfo *) malloc(txf->num_glyphs * sizeof(TexGlyphInfo));
	if (txf->tgi == NULL) {
		lastError = "out of memory.";
		goto error;
	}
	/*CONSTANTCONDITION*/ assert(sizeof(TexGlyphInfo) == 12);	/* Ensure external file format size. */
	got = fread(txf->tgi, sizeof(TexGlyphInfo), txf->num_glyphs, file);
	EXPECT(txf->num_glyphs);

	if (swap) {
		for (i = 0; i < txf->num_glyphs; i++) {
			SWAPS(&txf->tgi[i].c, tmp);
			SWAPS(&txf->tgi[i].x, tmp);
			SWAPS(&txf->tgi[i].y, tmp);
		}
	}
	txf->tgvi = (TexGlyphVertexInfo *)
		malloc(txf->num_glyphs * sizeof(TexGlyphVertexInfo));
	if (txf->tgvi == NULL) {
		lastError = "out of memory.";
		goto error;
	}
	w = txf->tex_width;
	h = txf->tex_height;
	xstep = 0.5 / w;
	ystep = 0.5 / h;
        txf->max_width = -1; // Celestia addition
	for (i = 0; i < txf->num_glyphs; i++) {
		TexGlyphInfo *tgi;

		tgi = &txf->tgi[i];
		txf->tgvi[i].t0[0] = tgi->x / w + xstep;
		txf->tgvi[i].t0[1] = tgi->y / h + ystep;
		txf->tgvi[i].v0[0] = tgi->xoffset;
		txf->tgvi[i].v0[1] = tgi->yoffset;
		txf->tgvi[i].t1[0] = (tgi->x + tgi->width) / w + xstep;
		txf->tgvi[i].t1[1] = tgi->y / h + ystep;
		txf->tgvi[i].v1[0] = tgi->xoffset + tgi->width;
		txf->tgvi[i].v1[1] = tgi->yoffset;
		txf->tgvi[i].t2[0] = (tgi->x + tgi->width) / w + xstep;
		txf->tgvi[i].t2[1] = (tgi->y + tgi->height) / h + ystep;
		txf->tgvi[i].v2[0] = tgi->xoffset + tgi->width;
		txf->tgvi[i].v2[1] = tgi->yoffset + tgi->height;
		txf->tgvi[i].t3[0] = tgi->x / w + xstep;
		txf->tgvi[i].t3[1] = (tgi->y + tgi->height) / h + ystep;
		txf->tgvi[i].v3[0] = tgi->xoffset;
		txf->tgvi[i].v3[1] = tgi->yoffset + tgi->height;
		txf->tgvi[i].advance = tgi->advance;

                if (tgi->width > txf->max_width)
                    txf->max_width = tgi->width; // Celestia addition
	}

	min_glyph = txf->tgi[0].c;
	max_glyph = txf->tgi[0].c;
	for (i = 1; i < txf->num_glyphs; i++) {
		if (txf->tgi[i].c < min_glyph) {
			min_glyph = txf->tgi[i].c;
		}
		if (txf->tgi[i].c > max_glyph) {
			max_glyph = txf->tgi[i].c;
		}
	}
	txf->min_glyph = min_glyph;
	txf->range = max_glyph - min_glyph + 1;

	txf->lut = (TexGlyphVertexInfo **)
		calloc(txf->range, sizeof(TexGlyphVertexInfo *));
	if (txf->lut == NULL) {
		lastError = "out of memory.";
		goto error;
	}
	for (i = 0; i < txf->num_glyphs; i++) {
		txf->lut[txf->tgi[i].c - txf->min_glyph] = &txf->tgvi[i];
	}

	switch (format) {
	case TXF_FORMAT_BYTE:
            printf("TXT_FORMAT_BYTE\n");
		if (useLuminanceAlpha) {
			unsigned char *orig;

			orig =
				(unsigned char *) malloc(txf->tex_width * txf->tex_height);
			if (orig == NULL) {
				lastError = "out of memory.";
				goto error;
			}
			got = fread(orig, 1, txf->tex_width * txf->tex_height, file);
			EXPECT(txf->tex_width * txf->tex_height);
			txf->teximage = (unsigned char *)
				malloc(2 * txf->tex_width * txf->tex_height);
			if (txf->teximage == NULL) {
				lastError = "out of memory.";
				goto error;
			}
			for (i = 0; i < txf->tex_width * txf->tex_height; i++) {
				txf->teximage[i * 2] = orig[i];
				txf->teximage[i * 2 + 1] = orig[i];
			}
			free(orig);
		} else {
			txf->teximage = (unsigned char *)
				malloc(txf->tex_width * txf->tex_height);
			if (txf->teximage == NULL) {
				lastError = "out of memory.";
				goto error;
			}
			got =
				fread(txf->teximage, 1, txf->tex_width * txf->tex_height,
					  file);
			EXPECT(txf->tex_width * txf->tex_height);
		}
		break;
	case TXF_FORMAT_BITMAP:
            printf("TXF_FORMAT_BITMAP\n");
		width = txf->tex_width;
		height = txf->tex_height;
		stride = (width + 7) >> 3;
		texbitmap = (unsigned char *) malloc(stride * height);
		if (texbitmap == NULL) {
			lastError = "out of memory.";
			goto error;
		}
		got = fread(texbitmap, 1, stride * height, file);
		EXPECT(stride * height);
		if (useLuminanceAlpha) {
			txf->teximage =
				(unsigned char *) calloc(width * height * 2, 1);
			if (txf->teximage == NULL) {
				lastError = "out of memory.";
				goto error;
			}
			for (i = 0; i < height; i++) {
				for (j = 0; j < width; j++) {
					if (texbitmap[i * stride + (j >> 3)] & (1 << (j & 7))) {
						txf->teximage[(i * width + j) * 2] = 255;
						txf->teximage[(i * width + j) * 2 + 1] = 255;
					}
				}
			}
		} else {
			txf->teximage = (unsigned char *) calloc(width * height, 1);
			if (txf->teximage == NULL) {
				lastError = "out of memory.";
				goto error;
			}
			for (i = 0; i < height; i++) {
				for (j = 0; j < width; j++) {
					if (texbitmap[i * stride + (j >> 3)] & (1 << (j & 7))) {
						txf->teximage[i * width + j] = 255;
					}
				}
			}
		}
		free(texbitmap);
		break;
	}

	fclose(file);
	return txf;

  error:

	if (txf) {
		if (txf->tgi)
			free(txf->tgi);
		if (txf->tgvi)
			free(txf->tgvi);
		if (txf->lut)
			free(txf->lut);
		if (txf->teximage)
			free(txf->teximage);
		free(txf);
	}
	if (file)
		fclose(file);
	return NULL;
}

GLuint
txfEstablishTexture(TexFont * txf, GLuint texobj, GLboolean setupMipmaps)
{
	if (txf->texobj == 0) {
		if (texobj == 0) {
#if !defined(USE_DISPLAY_LISTS)
			glGenTextures(1, &txf->texobj);
#else
			txf->texobj = glGenLists(1);
#endif
		} else {
			txf->texobj = texobj;
		}
	}
#if !defined(USE_DISPLAY_LISTS)
	glBindTexture(GL_TEXTURE_2D, txf->texobj);
#else
	glNewList(txf->texobj, GL_COMPILE);
#endif

	if (useLuminanceAlpha) {
		if (setupMipmaps) {
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE_ALPHA,
							  txf->tex_width, txf->tex_height,
							  GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
							  txf->teximage);
		} else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
						 txf->tex_width, txf->tex_height, 0,
						 GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
						 txf->teximage);
		}
	} else {
#if defined(GL_VERSION_1_1) || defined(GL_EXT_texture)
		/* Use GL_INTENSITY4 as internal texture format since we want to use as
		   little texture memory as possible. */
		if (setupMipmaps) {
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_INTENSITY4,
							  txf->tex_width, txf->tex_height,
							  GL_LUMINANCE, GL_UNSIGNED_BYTE,
							  txf->teximage);
		} else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_INTENSITY4,
						 txf->tex_width, txf->tex_height, 0,
						 GL_LUMINANCE, GL_UNSIGNED_BYTE, txf->teximage);
		}
#else
		abort();				/* Should not get here without EXT_texture or OpenGL
								   1.1. */
#endif
	}

#if defined(USE_DISPLAY_LISTS)
	glEndList();
	glCallList(txf->texobj);
#endif
	return txf->texobj;
}

void txfBindFontTexture(TexFont * txf)
{
#if !defined(USE_DISPLAY_LISTS)
	glBindTexture(GL_TEXTURE_2D, txf->texobj);
#else
	glCallList(txf->texobj);
#endif
}

void txfUnloadFont(TexFont * txf)
{
	if (txf->teximage) {
		free(txf->teximage);
	}
	free(txf->tgi);
	free(txf->tgvi);
	free(txf->lut);
	free(txf);
}

void
txfGetStringMetrics(TexFont* txf,
                    char* s, int len,
                    int& width, int& maxAscent, int& maxDescent)
{
    TexGlyphVertexInfo *tgvi;
    int w = 0;

    for (int i = 0; i < len; i++)
    {
        tgvi = getTCVI(txf, s[i]);
        w += (int) tgvi->advance;
    }

    width = w;
    maxAscent = txf->max_ascent;
    maxDescent = txf->max_descent;
}

void
txfGetStringMetrics(TexFont* txf,
                    string s,
                    int& width, int& maxAscent, int& maxDescent)
{
    TexGlyphVertexInfo *tgvi;
    int w = 0;

    for (unsigned int i = 0; i < s.length(); i++)
    {
        tgvi = getTCVI(txf, s[i]);
        w += (int) tgvi->advance;
    }

    width = w;
    maxAscent = txf->max_ascent;
    maxDescent = txf->max_descent;
}


void txfRenderGlyph(TexFont * txf, int c)
{
	TexGlyphVertexInfo *tgvi;

	tgvi = getTCVI(txf, c);
	glBegin(GL_QUADS);
        glTexCoord2fv(tgvi->t0);
	// glVertex2sv(tgvi->v0);
        glVertex2f(tgvi->v0[0], tgvi->v0[1]);
        glTexCoord2fv(tgvi->t1);
	// glVertex2sv(tgvi->v1);
        glVertex2f(tgvi->v1[0], tgvi->v1[1]);
        glTexCoord2fv(tgvi->t2);
	glVertex2sv(tgvi->v2);
	glTexCoord2fv(tgvi->t3);
	glVertex2sv(tgvi->v3);
	glEnd();
	glTranslatef(tgvi->advance, 0.0, 0.0);
}


void txfRenderString(TexFont * txf, char *s, int len)
{
    for (unsigned int i = 0; i < len; i++)
        txfRenderGlyph(txf, s[i]);
}

void txfRenderString(TexFont * txf, string s)
{
    for (unsigned int i = 0; i < s.length(); i++)
        txfRenderGlyph(txf, s[i]);
}


int txfInFont(TexFont * txf, int c)
{
    /* NOTE: No uppercase/lowercase substituion. */
    if ((c >= txf->min_glyph) && (c < txf->min_glyph + txf->range)) {
        if (txf->lut[c - txf->min_glyph]) {
            return 1;
        }
    }
    return 0;
}
