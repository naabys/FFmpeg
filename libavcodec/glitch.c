
#include "libavutil/opt.h"
#include "avcodec.h"
#include "internal.h"
#include "put_bits.h"

int ff_glitch_encode_init(AVCodecContext *avctx);
int ff_glitch_encode_end(AVCodecContext *avctx);
int ff_glitch_encode_picture(AVCodecContext *avctx, AVPacket *pkt,
                             const AVFrame *frame, int *got_packet);

typedef struct GlitchContext {
    AVClass *class;

    const char *dump_mjpeg_dc;
    const char *read_mjpeg_dc;
} GlitchContext;

PutBitContext glitch_pb;
FILE *glitch_dump_mjpeg_dc;
int glitch_read_mjpeg_dc;
static int mb_height, mb_width, nb_components, nb_blocks;
static int *dc_array;
static int dc_offset(int mb_y, int mb_x, int i, int j)
{
    return ((mb_y * mb_width + mb_x) * nb_components + i) * nb_blocks + j;
}
int glitch_get_mjpeg_dc(int mb_y, int mb_x, int i, int j);
int glitch_get_mjpeg_dc(int mb_y, int mb_x, int i, int j)
{
    return dc_array[dc_offset(mb_y, mb_x, i, j)];
}

int ff_glitch_encode_picture(AVCodecContext *avctx, AVPacket *pkt,
                             const AVFrame *pic_arg, int *got_packet)
{
    int size = (put_bits_count(&glitch_pb) + 7) >> 3;
    int ret = ff_alloc_packet2(avctx, pkt, size, size);
    if (ret < 0)
        return ret;
    memcpy(pkt->data, glitch_pb.buf, size);

//    pkt->flags |= AV_PKT_FLAG_KEY;
    *got_packet = 1;

    return 0;
}

av_cold int ff_glitch_encode_init(AVCodecContext *avctx)
{
    GlitchContext *s = avctx->priv_data;

    if ( s->dump_mjpeg_dc != NULL )
        glitch_dump_mjpeg_dc = fopen(s->dump_mjpeg_dc, "w");
    if ( s->read_mjpeg_dc != NULL )
    {
        int mb_y, mb_x, i, j, val;
        FILE *fp = fopen(s->read_mjpeg_dc, "r");
        if ( fp == NULL )
        {
            av_log(avctx, AV_LOG_ERROR, "Could not open '%s'\n", s->read_mjpeg_dc);
            return AVERROR(EINVAL);
        }
        fscanf(fp, "%d %d %d %d\n", &mb_height, &mb_width, &nb_components, &nb_blocks);
        dc_array = av_malloc(mb_height * mb_width * nb_components * nb_blocks * sizeof(int));
        while ( fscanf(fp, "%d %d %d %d %d\n", &mb_y, &mb_x, &i, &j, &val) != EOF )
            dc_array[dc_offset(mb_y, mb_x, i, j)] = val;
        glitch_read_mjpeg_dc = 1;
    }

    return 0;
}

av_cold int ff_glitch_encode_end(AVCodecContext *avctx)
{
    if ( glitch_dump_mjpeg_dc != NULL )
        fclose(glitch_dump_mjpeg_dc);
    if ( dc_array != NULL )
        av_freep(&dc_array);

    return 0;
}

#define OFFSET(x) offsetof(GlitchContext, x)
#define VE AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM
static const AVOption options[] = {
{ "dump_mjpeg_dc", "dump mjpeg dc coeffs", OFFSET(dump_mjpeg_dc), AV_OPT_TYPE_STRING, { .str = NULL }, 0, 0, VE },
{ "read_mjpeg_dc", "read mjpeg dc coeffs", OFFSET(read_mjpeg_dc), AV_OPT_TYPE_STRING, { .str = NULL }, 0, 0, VE },
{ NULL },
};

static const AVClass mjpeg_glitch_class = {
    .class_name = "mjpeg encoder - glitch",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVCodec ff_mjpeg_glitch_encoder = {
    .name           = "mjpeg_glitch",
    .long_name      = NULL_IF_CONFIG_SMALL("MJPEG (Motion JPEG) - glitch"),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_MJPEG,
    .priv_data_size = sizeof(GlitchContext),
    .init           = ff_glitch_encode_init,
    .encode2        = ff_glitch_encode_picture,
    .close          = ff_glitch_encode_end,
    .capabilities   = AV_CODEC_CAP_SLICE_THREADS | AV_CODEC_CAP_FRAME_THREADS | AV_CODEC_CAP_INTRA_ONLY,
    .pix_fmts       = (const enum AVPixelFormat[]){
        AV_PIX_FMT_YUVJ420P, AV_PIX_FMT_YUVJ422P, AV_PIX_FMT_YUVJ444P, AV_PIX_FMT_NONE
    },
    .priv_class     = &mjpeg_glitch_class,
};
