// Header section
#ifndef _OGGTHEORACAPTURE_H_
#define _OGGTHEORACAPTURE_H_

#include "theora/theora.h"
#include "moviecapture.h"

class OggTheoraCapture : public MovieCapture
{
public:
    OggTheoraCapture();
    virtual ~OggTheoraCapture();

    bool start(const std::string& filename, int w, int h, float fps);
    bool end();
    bool captureFrame();

    int getWidth() const;
    int getHeight() const;
    float getFrameRate() const;
    int getFrameCount() const;
    int getBytesOut() const { return video_bytesout; } ;
    void setAspectRatio(int, int);
    void setQuality(float);
    void recordingStatus(bool) {};  // Added to allow GTK compilation

private:
    void cleanup();

private:
    int video_x;
    int video_y;
    int frame_x;
    int frame_y;
    int frame_x_offset;
    int frame_y_offset;
    int video_an;
    int video_ad;
    int video_hzn;
    int video_hzd;
    int video_r; // 45000 <= video_r <= 2000000 (45Kbps - 2000Kbps)
    int video_q; // 0-63 aka 0-10 * 6.3

    bool        capturing;
    int        video_frame_count;
    int        video_bytesout;

    // Consider RGB to YUV Color converstion table - jpeglib has one
    // but according the standards it's incorrect (generates values 0-255,
    // instead of clamped to 16-240).

    int        rowStride;
    unsigned char  *pixels;
    unsigned char  *yuvframe[2];
    yuv_buffer    yuv;
    FILE           *outfile;
    ogg_stream_state to; /* take physical pages, weld into a logical
                stream of packets */
    ogg_page         videopage; /* one Ogg bitstream page. Theora packets are inside */
    ogg_packet       op; /* one raw packet of data for encode */

    theora_state     td;
    theora_info      ti;
    theora_comment   tc;

    virtual void frameCaptured() {}; /* to update UI status indicator */
};
#endif // _OGGTHEORACAPTURE_H_
