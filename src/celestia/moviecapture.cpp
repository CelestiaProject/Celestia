#define AVCODEC_DEBUG 0

#include <iostream>
#include <vector>
#include <fmt/format.h>

#define __STDC_CONSTANT_MACROS
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/timestamp.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <celengine/render.h>
#include "moviecapture.h"

using namespace std;

namespace celestia
{
// a wrapper around a single output AVStream
class MovieCapturePrivate
{
    MovieCapturePrivate() = default;
    ~MovieCapturePrivate();

    bool init(const fs::path& fn);
    bool addStream(int w, int h, float fps);
    bool openVideo();
    bool start();
    bool writeVideoFrame(bool = false);
    void finish();
    void setVideoCodec(int);

    bool isSupportedPixelFormat(enum AVPixelFormat) const;

    int writePacket();

    AVStream        *st       { nullptr };
    AVFrame         *frame    { nullptr };
    AVFrame         *tmpfr    { nullptr };
    AVCodecContext  *enc      { nullptr };
    AVFormatContext *oc       { nullptr };
    const AVCodec   *vc       { nullptr };
    AVPacket        *pkt      { nullptr };
    SwsContext      *swsc     { nullptr };

    const Renderer  *renderer { nullptr };

    // pts of the next frame that will be generated
    int64_t         nextPts   { 0       };
    // requested bitrate
    int64_t         bitrate  { 400000  };

    AVCodecID       vc_id     { AV_CODEC_ID_FFVHUFF };
    AVPixelFormat   format    { AV_PIX_FMT_NONE     };
    float           fps       { 0       };
    bool            capturing { false   };
    bool            hasAlpha  { false   };

    fs::path        filename;
    std::string     vc_options;

 public:
#if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 10, 100)) // ffmpeg < 4.0
    static bool     registered;
#endif
    friend class MovieCapture;
};

#if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 10, 100)) // ffmpeg < 4.0
bool MovieCapturePrivate::registered = false;
#endif

bool MovieCapturePrivate::init(const fs::path& filename)
{
    this->filename = filename;

#if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 10, 100)) // ffmpeg < 4.0
    if (!MovieCapturePrivate::registered)
    {
        av_register_all();
        MovieCapturePrivate::registered = true;
    }
#endif

    // always use matroska (*.mkv) as a container
    // don't change filename.string().c_str() -> filename.c_str()!
    // on windows c_str() return wchar_t*
    avformat_alloc_output_context2(&oc, nullptr, "matroska", filename.string().c_str());

    return oc != nullptr;
}

bool MovieCapturePrivate::isSupportedPixelFormat(enum AVPixelFormat format) const
{
    const enum AVPixelFormat *p = vc->pix_fmts;
    if (p == nullptr)
        return false;

    for (; *p != -1; p++)
    {
        if (*p == format)
            return true;
    }

    return false;
}

#if AVCODEC_DEBUG
static const char* to_str(AVOptionType type)
{
    switch(type)
    {
    case AV_OPT_TYPE_INT:
        return "int";
    case AV_OPT_TYPE_INT64:
        return "int64";
    case AV_OPT_TYPE_DOUBLE:
        return "double";
    case AV_OPT_TYPE_FLOAT:
        return "float";
    case AV_OPT_TYPE_STRING:
        return "string";
    case AV_OPT_TYPE_BINARY:
        return "binary";
    default:
        return "other";
    }
}

static void listCodecOptions(const AVCodecContext *enc)
{
    const AVOption *opt = nullptr;
    cout << "supported options:\n";
    while ((opt = av_opt_next(enc->priv_data, opt)) != nullptr)
    {
        if (opt->type == AV_OPT_TYPE_CONST)
        {
            fmt::print("\tname: {}\n", opt->name);
        }
        else
        {
            fmt::print("\tname: {}, type: {}, help: {}, min: {}, max: {}\n",
                       opt->name, to_str(opt->type), opt->help, opt->min, opt->max);
        }
    }
}

static void listEncoderParameters(const AVCodec *vc)
{
    fmt::print("codec: {} ({})\n", vc->name, vc->long_name);

    cout << "supported framerates:\n";
    const AVRational *f = vc->supported_framerates;
    if (f != nullptr)
    {
        for (; f->num != 0 && f->den != 0; f++)
            fmt::print("\t{} {}\n", f->num, f->den);
    }
    else
    {
        cout << "\tany\n";
    }

    cout << "supported pixel formats:\n";
    const enum AVPixelFormat *p = vc->pix_fmts;
    if (p != nullptr)
    {
        for (; *p != -1; p++)
            fmt::print("\t{}\n", av_pix_fmt_desc_get(*p)->name);
    }
    else
    {
        cout << "\tunknown\n";
    }

    cout << "recognized profiles:\n";
    const AVProfile *r = vc->profiles;
    if (r != nullptr)
    {
        for (; r->profile != FF_PROFILE_UNKNOWN; r++)
            fmt::print("\t{} {}\n", r->profile, r->name);
    }
    else
    {
        cout << "\tunknown\n";
    }
}
#endif

int MovieCapturePrivate::writePacket()
{
    // rescale output packet timestamp values from codec to stream timebase
    av_packet_rescale_ts(pkt, enc->time_base, st->time_base);
    pkt->stream_index = st->index;

    // Write the compressed frame to the media file.
    return av_interleaved_write_frame(oc, pkt);
}

// add an output stream
bool MovieCapturePrivate::addStream(int width, int height, float fps)
{
    this->fps = fps;

    // find the encoder
    vc = avcodec_find_encoder(vc_id);
    if (vc == nullptr)
    {
        cout << "Video codec isn't found\n";
        return false;
    }

#if AVCODEC_DEBUG
    listEncoderParameters(vc);
#endif

    st = avformat_new_stream(oc, nullptr);
    if (st == nullptr)
    {
        cout << "Unable to alloc a new stream\n";
        return false;
    }
    st->id = oc->nb_streams - 1;

    enc = avcodec_alloc_context3(vc);
    if (enc == nullptr)
    {
        cout << "Unable to alloc a new context\n";
        return false;
    }

    enc->codec_id = vc_id;
    enc->bit_rate = bitrate;
#if 0
    enc->rc_min_rate = ...;
    enc->rc_max_rate = ...;
    enc->bitrate_tolerance = 0;
#endif
    // Resolution must be a multiple of two
    enc->width     = width;
    enc->height    = height;
    // timebase: This is the fundamental unit of time (in seconds) in terms
    // of which frame timestamps are represented. For fixed-fps content,
    // timebase should be 1/framerate and timestamp increments should be
    // identical to 1.
    if (abs(fps - 29.97f) < 1e-5f)
        st->time_base = { 1001, 30000 };
    else if (abs(fps - 23.976f) < 1e-5f)
        st->time_base = { 1001, 24000 };
    else
        st->time_base = { 1, (int) fps };

    enc->time_base = st->time_base;
    enc->framerate = st->avg_frame_rate = { st->time_base.den, st->time_base.num };
    enc->gop_size  = 12; // emit one intra frame every twelve frames at most

    // find a best pixel format to convert to from `format`
    if (isSupportedPixelFormat(AV_PIX_FMT_YUV420P))
    {
        enc->pix_fmt = AV_PIX_FMT_YUV420P;
    }
    else
    {
        enc->pix_fmt = avcodec_find_best_pix_fmt_of_list(vc->pix_fmts, format, 0, nullptr);
        if (enc->pix_fmt == AV_PIX_FMT_NONE)
            avcodec_default_get_format(enc, &(enc->pix_fmt));
    }

    if (enc->codec_id == AV_CODEC_ID_MPEG1VIDEO)
    {
        // Need to avoid usage of macroblocks in which some coeffs overflow.
        // This does not happen with normal video, it just happens here as
        // the motion of the chroma plane does not match the luma plane.
        enc->mb_decision = 2;
    }

    // Some formats want stream headers to be separate.
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

#if AVCODEC_DEBUG
    listCodecOptions(enc);
#endif
    return true;
}

bool MovieCapturePrivate::start()
{
    // open the output file, if needed
    if ((oc->oformat->flags & AVFMT_NOFILE) == 0)
    {
        if (avio_open(&oc->pb, filename.string().c_str(), AVIO_FLAG_WRITE) < 0)
        {
            cout << "Failed to open video file\n";
            return false;
        }
    }

    // Write the stream header, if any.
    if (avformat_write_header(oc, nullptr) < 0)
    {
        cout << "Failed to write header\n";
        return false;
    }

    av_dump_format(oc, 0, filename.string().c_str(), 1);

    if ((pkt = av_packet_alloc()) == nullptr)
    {
        cout << "Failed to allocate a packet\n";
        return false;
    }

    return true;
}

bool MovieCapturePrivate::openVideo()
{
    AVDictionary *opts = nullptr;
    const char *str = "";

    if (av_dict_parse_string(&opts, vc_options.c_str(), "=", ",", 0) != 0)
        cout << "Failed to parse error codec parameters\n";

    // open the codec
    if (avcodec_open2(enc, vc, &opts) < 0)
    {
        cout << "Failed to open the codec\n";
        av_dict_free(&opts);
        return false;
    }

    if (av_dict_count(opts) > 0)
    {
        cout << "Unrecognized options:\n";
        AVDictionaryEntry *t = nullptr;
        while ((t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)) != nullptr)
            fmt::print("\t{}={}\n", t->key, t->value);
    }
    av_dict_free(&opts);

    // allocate and init a re-usable frame
    if ((frame = av_frame_alloc()) == nullptr)
    {
        cout << "Failed to allocate destination frame\n";
        return false;
    }

    frame->format = enc->pix_fmt;
    frame->width  = enc->width;
    frame->height = enc->height;

    // allocate the buffers for the frame data
    if (av_frame_get_buffer(frame, 32) < 0)
    {
        cout << "Failed to allocate destination frame buffer\n";
        return false;
    }

    if (enc->pix_fmt != format)
    {
        // as we only grab a RGB24 picture, we must convert it
        // to the codec pixel format if needed
        swsc = sws_getContext(enc->width, enc->height, format,
                              enc->width, enc->height, enc->pix_fmt,
                              SWS_BITEXACT, nullptr, nullptr, nullptr);
        if (swsc == nullptr)
        {
            cout << "Failed to allocate SWS context\n";
            return false;
        }

        // allocate and init a temporary frame
        if((tmpfr = av_frame_alloc()) == nullptr)
        {
            cout << "Failed to allocate temp frame\n";
            return false;
        }

        tmpfr->format = format;
        tmpfr->width  = enc->width;
        tmpfr->height = enc->height;

        // allocate the buffers for the frame data
        if (av_frame_get_buffer(tmpfr, 32) < 0)
        {
            cout << "Failed to allocate temp frame buffer\n";
            return false;
        }
    }

    // copy the stream parameters to the muxer
    if (avcodec_parameters_from_context(st->codecpar, enc) < 0)
    {
        cout << "Failed to copy the stream parameters to the muxer\n";
        return false;
    }

    return true;
}

static void captureImage(AVFrame *pict, int width, int height, const Renderer *r)
{
    int x, y, w, h;
    r->getViewport(&x, &y, &w, &h);

    x += (w - width) / 2;
    y += (h - height) / 2;
    r->captureFrame(x, y, width, height,
                    r->getPreferredCaptureFormat(),
                    pict->data[0]);
}

// encode one video frame and send it to the muxer
// return 1 when encoding is finished, 0 otherwise
bool MovieCapturePrivate::writeVideoFrame(bool finalize)
{
    AVFrame *frame = finalize ? nullptr : this->frame;
    const int bytesPerPixel = hasAlpha ? 4 : 3;

    // check if we want to generate more frames
    if (!finalize)
    {
        // when we pass a frame to the encoder, it may keep a reference to it
        // internally; make sure we do not overwrite it here
        if (av_frame_make_writable(frame) < 0)
        {
            cout << "Failed to make the frame writable\n";
            return false;
        }

        if (enc->pix_fmt != format)
        {
            captureImage(tmpfr, enc->width, enc->height, renderer);
            // we need to compute the correct line width of our source data
            const int linesize = bytesPerPixel * enc->width;
            sws_scale(swsc, tmpfr->data, &linesize, 0, enc->height,
                      frame->data, frame->linesize);
        }
        else
        {
            captureImage(frame, enc->width, enc->height, renderer);
        }

        frame->pts = nextPts++;
    }

#if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 133, 100))
    av_init_packet(pkt);
#endif

    // encode the image
    if (avcodec_send_frame(enc, frame) < 0)
    {
        cout << "Failed to send the frame\n";
        return false;
    }

    for (;;)
    {
        int ret = avcodec_receive_packet(enc, pkt);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;

        if (ret >= 0)
        {
            ret = writePacket();
            av_packet_unref(pkt);
        }

        if (ret < 0)
        {
            cout << "Failed to receive/unref the packet\n";
            return false;
        }
    }

    return true;
}

void MovieCapturePrivate::finish()
{
    writeVideoFrame(true);

    // Write the trailer, if any. The trailer must be written before you
    // close the CodecContexts open when you wrote the header; otherwise
    // av_write_trailer() may try to use memory that was freed on
    // av_codec_close().
    av_write_trailer(oc);

    if (!(oc->oformat->flags & AVFMT_NOFILE))
        avio_closep(&oc->pb);
}

MovieCapturePrivate::~MovieCapturePrivate()
{
    avcodec_free_context(&enc);
    av_frame_free(&frame);
    if (tmpfr != nullptr)
        av_frame_free(&tmpfr);
    avformat_free_context(oc);
    av_packet_free(&pkt);
}

MovieCapture::MovieCapture(const Renderer *r) :
    d(new MovieCapturePrivate)
{
    d->renderer = r;
    d->hasAlpha = r->getPreferredCaptureFormat() == PixelFormat::RGBA;
    d->format   = d->hasAlpha ? AV_PIX_FMT_RGBA : AV_PIX_FMT_RGB24;
}

MovieCapture::~MovieCapture()
{
    delete d;
}

int MovieCapture::getFrameCount() const
{
    return d->nextPts;
}

int MovieCapture::getWidth() const
{
    return d->enc->width;
}

int MovieCapture::getHeight() const
{
    return d->enc->height;
}

float MovieCapture::getFrameRate() const
{
    return d->fps;
}

bool MovieCapture::start(const fs::path& filename, int width, int height, float fps)
{
    if (!d->init(filename) ||
        !d->addStream(width, height, fps) ||
        !d->openVideo() ||
        !d->start())
    {
        return false;
    }

    d->capturing = true; // XXX

    return true;
}

bool MovieCapture::end()
{
    if (!d->capturing)
        return false;

    d->finish();

    d->capturing = false;

    return true;
}

bool MovieCapture::captureFrame()
{
    return d->capturing && d->writeVideoFrame();
}

void MovieCapture::setVideoCodec(AVCodecID vc_id)
{
    d->vc_id = vc_id;
}

void MovieCapture::setBitRate(int64_t bitrate)
{
    d->bitrate = bitrate;
}

void MovieCapture::setEncoderOptions(const std::string &s)
{
    d->vc_options = s;
}
}
