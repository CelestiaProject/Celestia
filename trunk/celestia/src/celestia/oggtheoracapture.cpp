/*
 *  Copyright (C) 2006, William K Volkman <wkvsf@users.sourceforge.net>
 * 
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 * http://www.fourcc.org/fccyvrgb.php
 * Ey = 0.299R+0.587G+0.114B
 * Ecr = 0.713(R - Ey) = 0.500R-0.419G-0.081B
 * Ecb = 0.564(B - Ey) = -0.169R-0.331G+0.500B
 *
 * the defined range for Y is [16,235] (220 steps) and the valid ranges
 * for Cr and Cb are [16,239] (235 steps) 
 *
 * http://www.neuro.sfc.keio.ac.jp/~aly/polygon/info/color-space-faq.html
 *  RGB -> YUV                    | YUV -> RGB
 *  Y =  0.299*Red+0.587*Green+0.114*Blue    | Red   = Y+0.000*U+1.140*V
 *  U = -0.147*Red-0.289*Green+0.436*Blue    | Green = Y-0.396*U-0.581*V
 *  V =  0.615*Red-0.515*Green-0.100*Blue    | Blue  = Y+2.029*U+0.000*V
 *
 *  +----------------+---------------+-----------------+----------------+
 *  | Recommendation | Coef. for red | Coef. for Green | Coef. for Blue |
 *  +----------------+---------------+-----------------+----------------+
 *  | Rec 601-1      | 0.299         | 0.587           | 0.114          |
 *  | Rec 709        | 0.2125        | 0.7154          | 0.0721         |
 *  | ITU            | 0.2125        | 0.7154          | 0.0721         |
 *  +----------------+---------------+-----------------+----------------+
 *  RGB -> YCbCr
 *  Y  = Coef. for red*Red+Coef. for green*Green+Coef. for blue*Blue
 *  Cb = (Blue-Y)/(2-2*Coef. for blue)
 *  Cr = (Red-Y)/(2-2*Coef. for red)
 *
 *  RGB -> YCbCr (with Rec 601-1 specs)        | YCbCr (with Rec 601-1 specs) -> RGB
 *  Y=  0.2989*Red+0.5866*Green+0.1145*Blue    | Red=  Y+0.0000*Cb+1.4022*Cr
 *  Cb=-0.1687*Red-0.3312*Green+0.5000*Blue    | Green=Y-0.3456*Cb-0.7145*Cr
 *  Cr= 0.5000*Red-0.4183*Green-0.0816*Blue    | Blue= Y+1.7710*Cb+0.0000*Cr
 *
 * http://en.wikipedia.org/wiki/YUV/RGB_conversion_formulas
 * Y := min(abs(r * 2104 + g * 4130 + b * 802 + 4096 + 131072) >> 13,235)
 * U := min(abs(r * -1214 + g * -2384 + b * 3598 + 4096 + 1048576) >> 13,240)
 * V := min(abs(r * 3598 + g * -3013 + b * -585 + 4096 + 1048576) >> 13,240)
 */


#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef _REENTRANT
# define _REENTRANT
#endif

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <celutil/debug.h>
#include <celutil/util.h>
#include "../celengine/gl.h"
#include <string>
#include <cstring>
#include "theora/theora.h"

using namespace std;

#include "oggtheoracapture.h"

//  {"video-rate-target",required_argument,NULL,'V'},
//  {"video-quality",required_argument,NULL,'v'},
//  {"aspect-numerator",optional_argument,NULL,'s'},
//  {"aspect-denominator",optional_argument,NULL,'S'},
//  {"framerate-numerator",optional_argument,NULL,'f'},
//  {"framerate-denominator",optional_argument,NULL,'F'},


OggTheoraCapture::OggTheoraCapture():
    video_x(0),
    video_y(0),
    frame_x(0),
    frame_y(0),
    frame_x_offset(0),
    frame_y_offset(0),
    video_an(4),
    video_ad(3),
    video_hzn(12),
    video_hzd(1),
    video_r(-1), // 45000 <= video_r <= 2000000 (45Kbps - 2000Kbps)
    video_q(63), // 0-63 aka 0-10 * 6.3 the higher the value the faster the encoding and the larger the output file
    capturing(false),
    video_frame_count(0),
    video_bytesout(0),
    rowStride(0),
    pixels(NULL),
    outfile(NULL)
{
    yuvframe[0] = NULL;
    yuvframe[1] = NULL;
    // Just being anal
    memset(&yuv, 0, sizeof(yuv));
    memset(&to, 0, sizeof(to));
    memset(&videopage, 0, sizeof(videopage));
    memset(&op, 0, sizeof(op));
    memset(&td, 0, sizeof(td));
    memset(&ti, 0, sizeof(ti));
    memset(&tc, 0, sizeof(tc));
}

void OggTheoraCapture::setAspectRatio(int aspect_numerator, int aspect_denominator)
{
    int    a = aspect_numerator;
    int    b = aspect_denominator;
    while (a != b)
    {
        if (a > b)
            a = a - b;
        else
            b = b - a;
    }
    if (a > 1) {
        video_an = aspect_numerator / a;
        video_ad = aspect_denominator / a;
    } 
    else
    {
        video_an = aspect_numerator;
        video_ad = aspect_denominator;
    }
}
void OggTheoraCapture::setQuality(float quality)
{
    if (quality < 0.0)
        video_q = 7;
    else if (quality < 1.0)
        video_q = 0;
    else if (quality <= 10.00)
        video_q = (int)ceil(quality * 6.3);
    else
        video_q = (int)ceil(quality);
    
}
bool OggTheoraCapture::start(const std::string& filename,
                 int w, int h,
                 float fps)
{
    if (capturing)
        return false;

    outfile = fopen(filename.c_str(), "wb");
    if (!outfile)
    {
        DPRINTF(0, _("Error in creating ogg file %s for capture.\n"), filename.c_str());
        return false;
    }
    /* Set up Ogg output stream */
#ifdef _WIN32
	std::srand(std::time(NULL));
#else
	std::srand(time(NULL));
#endif

    ogg_stream_init(&to,std::rand());

    frame_x = w;
    frame_y = h;
    if (fps > 0.05) {
        if (fabs(fps - (30000.0/1001.0)) < 1e-5)
        {
            video_hzn = 30000;
            video_hzd = 1001;
        }
        else if (fabs(fps - (24000.0/1001.0)) < 1e-5)
        {
            video_hzn = 24000;
            video_hzd = 1001;
        }
        else
        {
            video_hzn = (int)ceil(fps*1000.0);
            video_hzd = 1000;
            int    a = video_hzn;
            int    b = video_hzd;
            while (a != b)
            {
                if (a > b)
                    a = a - b;
                else
                    b = b - a;
            }
            if (a > 1)
            {
                video_hzn /= a;
                video_hzd /= a;
            }
        }
    }

    /* Theora has a divisible-by-sixteen restriction for the encoded video size */
    /* scale the frame size up to the nearest /16 and calculate offsets */
    video_x=((frame_x + 15) >>4)<<4;
    video_y=((frame_y + 15) >>4)<<4;

    /* We force the offset to be even.
       This ensures that the chroma samples align properly with the luma
       samples. */
    frame_x_offset=((video_x-frame_x)/2)&~1;
    frame_y_offset=((video_y-frame_y)/2)&~1;

    theora_info_init(&ti);
    ti.width=video_x;
    ti.height=video_y;
    ti.frame_width=frame_x;
    ti.frame_height=frame_y;
    ti.offset_x=frame_x_offset;
    ti.offset_y=frame_y_offset;
    ti.fps_numerator=video_hzn;
    ti.fps_denominator=video_hzd;
    ti.aspect_numerator=video_an;
    ti.aspect_denominator=video_ad;
    if (frame_x == 720 && frame_y == 576)
        ti.colorspace=OC_CS_ITU_REC_470BG; //OC_CS_UNSPECIFIED;
    else
        ti.colorspace=OC_CS_ITU_REC_470M; //OC_CS_UNSPECIFIED;

    //ti.pixelformat=OC_PF_420;
    ti.target_bitrate=video_r;
    ti.quality=video_q;

    ti.dropframes_p=0;
    ti.quick_p=1;
    ti.keyframe_auto_p=1;
    ti.keyframe_frequency=64;
    ti.keyframe_frequency_force=64;
    ti.keyframe_data_target_bitrate=(int)(video_r*1.5);
    ti.keyframe_auto_threshold=80;
    ti.keyframe_mindistance=8;
    ti.noise_sensitivity=1;

    theora_encode_init(&td,&ti);
    theora_info_clear(&ti);

    /* first packet will get its own page automatically */
    theora_encode_header(&td,&op);
    ogg_stream_packetin(&to,&op);
    if(ogg_stream_pageout(&to,&videopage) != 1){
        cerr << _("Internal Ogg library error.") << endl;
        return false;
    }
    fwrite(videopage.header, 1, videopage.header_len, outfile);
    fwrite(videopage.body,   1, videopage.body_len, outfile);

    /* create the remaining theora headers */
    theora_comment_init(&tc);
    theora_encode_comment(&tc,&op);
    theora_comment_clear(&tc);
    ogg_stream_packetin(&to,&op);
    theora_encode_tables(&td,&op);
    ogg_stream_packetin(&to,&op);

    while(1)
    {
        int result = ogg_stream_flush(&to,&videopage);
        if( result<0 )
        {
            /* can't get here */
            cerr << _("Internal Ogg library error.")  << endl;
            return false;
        }
        if( result==0 )
            break;
        fwrite(videopage.header,1,videopage.header_len,outfile);
        fwrite(videopage.body,1,  videopage.body_len,outfile);
    }
    /* Initialize the double frame buffer.
     * We allocate enough for a 4:4:4 color sampling
     */
    yuvframe[0]= new unsigned char[video_x*video_y*3];
    yuvframe[1]= new unsigned char[video_x*video_y*3];

    // Now the buffer for reading the GL RGB pixels
    rowStride = (frame_x * 3 + 3) & ~0x3;
    pixels = new unsigned char[rowStride*frame_y];

        /* clear initial frame as it may be larger than actual video data */
        /* fill Y plane with 0x10 and UV planes with 0x80, for black data */
    // The UV plane must be 4:2:0
    memset(yuvframe[0],0x10,video_x*video_y);
    memset(yuvframe[0]+video_x*video_y,0x80,video_x*video_y*2);
    memset(yuvframe[1],0x10,video_x*video_y);
    memset(yuvframe[1]+video_x*video_y,0x80,video_x*video_y*2);

    yuv.y_width=video_x;
    yuv.y_height=video_y;
    yuv.y_stride=video_x;

    // Note we lie here by saying it's 4:2:0 and we will convert 4:4:4 to 4:2;0 later
    yuv.uv_width=video_x/2;
    yuv.uv_height=video_y/2;
    yuv.uv_stride=video_x/2;

    printf(_("OggTheoraCapture::start() - Theora video: %s %.2f(%d/%d) fps quality %d %dx%d offset (%dx%d)\n"),
           filename.c_str(),
           (double)video_hzn/(double)video_hzd,
           video_hzn,video_hzd,
           video_q,
           video_x,video_y,
           frame_x_offset,frame_y_offset);

    capturing = true;
    return true;
}

bool OggTheoraCapture::captureFrame()
{
    if (!capturing)
        return false;

    while (ogg_stream_pageout(&to,&videopage)>0)
    {
        /* flush a video page */
        video_bytesout+=fwrite(videopage.header,1,videopage.header_len,outfile);
        video_bytesout+=fwrite(videopage.body,1,videopage.body_len,outfile);

    }
    if(ogg_stream_eos(&to)) return false;

    // Get the dimensions of the current viewport
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    int x = viewport[0] + (viewport[2] - frame_x) / 2;
    int y = viewport[1] + (viewport[3] - frame_y) / 2;
    glReadPixels(x, y, frame_x, frame_y,
             GL_RGB, GL_UNSIGNED_BYTE,
             pixels);

    unsigned char *ybase = yuvframe[0];
    unsigned char *ubase = yuvframe[0]+ video_x*video_y;
    unsigned char *vbase = yuvframe[0]+ video_x*video_y*2;
    // We go ahead and build 4:4:4 frames
    for (int y=0; y<frame_y; y++)
    {
        unsigned char *yptr = ybase + (video_x*(y+frame_y_offset))+frame_x_offset;
        unsigned char *uptr = ubase + (video_x*(y+frame_y_offset))+frame_x_offset;
        unsigned char *vptr = vbase + (video_x*(y+frame_y_offset))+frame_x_offset;
        unsigned char *rgb = pixels + ((frame_y-1-y)*rowStride); // The video is inverted
        for (int x=0; x<frame_x; x++)
        {
            unsigned char r = *rgb++;
            unsigned char g = *rgb++;
            unsigned char b = *rgb++;
            *yptr++ = min(abs(r * 2104 + g * 4130 + b * 802 + 4096 + 131072) >> 13,235);
            *uptr++ = min(abs(r * -1214 + g * -2384 + b * 3598 + 4096 + 1048576) >> 13,240);
            *vptr++ = min(abs(r * 3598 + g * -3013 + b * -585 + 4096 + 1048576) >> 13,240);
        }
    }

    /*
     * The video strategy is to capture one frame ahead so when we're at end of
     * stream we can mark last video frame as such.  Have two YUV frames before
     * encoding. Theora is a one-frame-in,one-frame-out system; submit a frame
     * for compression and pull out the packet
     */

    if (video_frame_count > 0)
    {
        yuv.y= yuvframe[1];
        yuv.u= yuvframe[1]+ video_x*video_y;
        yuv.v= yuvframe[1]+ video_x*video_y*2;
        // Convert to 4:2:0
        unsigned char * uin0 = yuv.u;
        unsigned char * uin1 = yuv.u + video_x;
        unsigned char * uout = yuv.u;
        unsigned char * vin0 = yuv.v;
        unsigned char * vin1 = yuv.v + video_x;
        unsigned char * vout = yuv.v; 
        for (int y = 0; y < video_y; y += 2)
        {
            for (int x = 0; x < video_x; x += 2)
            {
                *uout = (uin0[0] + uin0[1] + uin1[0] + uin1[1]) >> 2;
                uin0 += 2;
                uin1 += 2;
                uout++;
                *vout = (vin0[0] + vin0[1] + vin1[0] + vin1[1]) >> 2;
                vin0 += 2;
                vin1 += 2;
                vout++;
            }
            uin0 += video_x;
            uin1 += video_x;
            vin0 += video_x;
            vin1 += video_x;
        }
        theora_encode_YUVin(&td,&yuv);
        theora_encode_packetout(&td,0,&op);
        ogg_stream_packetin(&to,&op);
    }
    video_frame_count += 1;
    //if ((video_frame_count % 10) == 0)
    //    printf("Writing frame %d\n", video_frame_count);
    unsigned char *temp = yuvframe[0];
    yuvframe[0] = yuvframe[1];
    yuvframe[1] = temp;
    frameCaptured();

    return true;
}
void OggTheoraCapture::cleanup()
{
    capturing = false;
    /* clear out state */

    if(outfile)
    {
        printf(_("OggTheoraCapture::cleanup() - wrote %d frames\n"), video_frame_count);
        if (video_frame_count > 0)
        {
            yuv.y= yuvframe[1];
            yuv.u= yuvframe[1]+ video_x*video_y;
            yuv.v= yuvframe[1]+ video_x*video_y*2 ;
            // Convert to 4:2:0
            unsigned char * uin0 = yuv.u;
            unsigned char * uin1 = yuv.u + video_x;
            unsigned char * uout = yuv.u;
            unsigned char * vin0 = yuv.v;
            unsigned char * vin1 = yuv.v + video_x;
            unsigned char * vout = yuv.v;
            for (int y = 0; y < video_y; y += 2)
            {
                for (int x = 0; x < video_x; x += 2)
                {
                    *uout = (uin0[0] + uin0[1] + uin1[0] + uin1[1]) >> 2;
                    uin0 += 2;
                    uin1 += 2;
                    uout++;
                    *vout = (vin0[0] + vin0[1] + vin1[0] + vin1[1]) >> 2;
                    vin0 += 2;
                    vin1 += 2;
                    vout++;
                }
                uin0 += video_x;
                uin1 += video_x;
                vin0 += video_x;
                vin1 += video_x;
            }
            theora_encode_YUVin(&td,&yuv);
            theora_encode_packetout(&td,1,&op);
            ogg_stream_packetin(&to,&op);
        }
        while(ogg_stream_pageout(&to,&videopage)>0)
        {
            /* flush a video page */
            video_bytesout+=fwrite(videopage.header,1,videopage.header_len,outfile);
            video_bytesout+=fwrite(videopage.body,1,videopage.body_len,outfile);

        }
        if(ogg_stream_flush(&to,&videopage)>0)
        {
            /* flush a video page */
            video_bytesout+=fwrite(videopage.header,1,videopage.header_len,outfile);
            video_bytesout+=fwrite(videopage.body,1,videopage.body_len,outfile);

        }
        theora_clear(&td);
        ogg_stream_clear(&to);
        //ogg_stream_destroy(&to); /* Documentation says to do this however we are seeing a double free libogg 1.1.2 */

        std::fclose(outfile);
        outfile = NULL;
        delete [] yuvframe[0];
        delete [] yuvframe[1];
        delete [] pixels;
    }
}

bool OggTheoraCapture::end()
{
    cleanup();
    return true;
}
int OggTheoraCapture::getWidth() const
{
    return frame_x;
}
int OggTheoraCapture::getHeight() const
{
    return frame_y;
}
int OggTheoraCapture::getFrameCount() const
{
    return video_frame_count;
}
float OggTheoraCapture::getFrameRate() const
{
    return float(video_hzn)/float(video_hzd);
}
OggTheoraCapture::~OggTheoraCapture()
{
    cleanup();
}

