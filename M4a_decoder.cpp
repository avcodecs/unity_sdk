// #include <stdlib.h>
// #include <assert.h>
// #include <math.h>
// #include "AudioDecoder.h"
// #include "AudioParams.h"

// //#ifdef CODEC_HAVE_SPEEX
// #include "speex_decoder.h"
// //#endif
// //#include "amrwbdecoder.h"

// //#ifdef CODEC_HAVE_MP3
// // UInt32 is also defined in apple_aac_decoder.h
// #define UInt32 UInt32_MP3
// #include "mp3_decoder.h"
// //#endif

// //#ifdef CODEC_HAVE_AAC
// //#include "pvaac.h"
// //#include "eaacdecoder.h"
// //#include "pvaac_decoder.h"
// #include "fdk_aac_decoder.h"
// #include "fdk_aac_encoder.h"
// //#ifdef YYMOBILE_IOS
// #include "apple_aac_decoder.h"
// //#endif
// //#endif

// //#ifdef CODEC_HAVE_SILK
// #include "silk_decoder.h"
// //#endif

// //#ifdef CODEC_HAVE_G729
// #include "g729_decoder.h"
// //#endif

// //#ifdef CODEC_HAVE_OPUS
// #include "opus121_decoder.h"
// //#endif


// #include "yyaudio.h"
// #include "ConvertPCM.h"
// #include "SdkConfig.h"
// #include "AudioPlayUnit.h"

// using namespace audiosdk;
// uint8_t g_audio_config[4];
// uint32_t g_audio_config_size;
// static uint32_t audio_sample_rate[16] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350,0,0,0 };

// enum AacHwCodecMgrCallbackType{
//     GET = 0,
//     UPDATE,
//     RELEASE
// };
#include "M4a_decoder.h"

namespace yymobile
{


CM4a_decoder::CM4a_decoder() :
	m_decoder(NULL)
    , mOutSampleRate(0)
    , mOutChannel(0)
    , mpResampler(NULL)
    , mIsUseHwCodec(false)
    , mAacHwCodecMgrCallback(NULL)
{
	//mStereoReampler = new CSpeexResampler();
}

CM4a_decoder::~CM4a_decoder() {
    
}



int CM4a_decoder::decode(const unsigned char *inputData, unsigned int length, unsigned char* outputData) {
	int ret = 0;
    int i = 0;
    int outLen = 0;
    int gotFrame = 0;
    AVPacket packet;
    AVIOBufferContext gAvbufferIn;
    uint8_t* inBuffer        = NULL;
    AVFrame *frame           = NULL;
    AVFrame *frameResampled  = NULL;
    AVCodecContext *codecCtx = NULL;
    AVFormatContext* ifmtCtx = NULL;
    AVIOContext *avioIn      = NULL;
    struct SwrContext *convertCtx = NULL;
    std::vector<uint8_t> resampledBuffer(AVIO_BUF_SIZE, 0);

    gAvbufferIn.totalSize = inLength;
    gAvbufferIn.ptr       = inData;
    gAvbufferIn.pos       = 0;

    //av_register_all();
    ifmtCtx  = avformat_alloc_context();
    inBuffer = (unsigned char*)av_malloc(AVIO_BUF_SIZE);
    avioIn   = avio_alloc_context(inBuffer, AVIO_BUF_SIZE, 0, &gAvbufferIn, readBuffer, NULL, NULL);
    if (ifmtCtx == NULL || inBuffer == NULL || avioIn == NULL) {
        *errorCode = RET_M4A_GENERIC_ERROR;
        goto m4aDecEnd;
    }
    ifmtCtx->pb    = avioIn;
    ifmtCtx->flags = AVFMT_FLAG_CUSTOM_IO;

    if ((ret = avformat_open_input(&ifmtCtx, NULL, NULL, NULL)) < 0) {
        *errorCode = RET_M4A_GENERIC_ERROR;
        goto m4aDecEnd;
    }

    if ((ret = avformat_find_stream_info(ifmtCtx, NULL)) < 0) {
        *errorCode = RET_M4A_GENERIC_ERROR;
        goto m4aDecEnd;
    }

    for (i = 0; i < ifmtCtx->nb_streams; i++) {
        codecCtx = avcodec_alloc_context3(NULL);
        if (codecCtx == NULL) {
            *errorCode = RET_M4A_GENERIC_ERROR;
            goto m4aDecEnd;
        }
        avcodec_parameters_to_context(codecCtx, ifmtCtx->streams[i]->codecpar);
        if (codecCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
            ret = avcodec_open2(codecCtx, avcodec_find_decoder(codecCtx->codec_id), NULL);
            if (ret < 0) {
                *errorCode = RET_M4A_GENERIC_ERROR;
                goto m4aDecEnd;
            }
        }
    }

    frame = av_frame_alloc();
    frameResampled = av_frame_alloc();
    if (!frame || !frameResampled) {
        *errorCode = RET_M4A_GENERIC_ERROR;
        goto m4aDecEnd;
    }

    convertCtx = swr_alloc();
    uint64_t outChannLayout;
    outChannLayout = (targetChannel == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
    uint64_t inChanLayout;
    inChanLayout = (codecCtx->channels == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
    convertCtx = swr_alloc_set_opts(convertCtx, outChannLayout, AV_SAMPLE_FMT_S16, targetSampleRate,
        inChanLayout, codecCtx->sample_fmt, codecCtx->sample_rate, 0, NULL);
    if (!convertCtx) {
        *errorCode = RET_M4A_GENERIC_ERROR;
        goto m4aDecEnd;
    }
    swr_init(convertCtx);
    frameResampled->format   = AV_SAMPLE_FMT_S16;
    frameResampled->channels = targetChannel;

    /* read all packets */
    while (1) {
        //解复用了，将媒体文件分开成音频流和视频流，同时也是将数据压缩到packet中
        if ((ret = av_read_frame(ifmtCtx, &packet)) < 0)
            break;
        int stream_index;
        stream_index = packet.stream_index;
        if (stream_index != 0)
            continue;
        ret = avcodec_decode_audio4(codecCtx, frame, &gotFrame, &packet);
        if (ret < 0) {
            *errorCode = RET_M4A_DECODE_ERROR;
            break;
        }

        if (gotFrame) {
            // *2 for safe
           frameResampled->nb_samples = frame->nb_samples * (targetSampleRate / codecCtx->sample_rate + 1) * 2;
            //frameResampled->nb_samples = av_rescale_rnd(frame->nb_samples, frameResampled->sample_rate, frame->sample_rate, AV_ROUND_UP);
            int resampledLineSize;
            resampledLineSize = 0;
            int resampledBuffSize;
            resampledBuffSize = av_samples_get_buffer_size(&resampledLineSize, frameResampled->channels, frameResampled->nb_samples, (AVSampleFormat)frameResampled->format, 0);
            if (resampledBuffer.size() < resampledBuffSize) {
                resampledBuffer.resize(resampledBuffSize, 0);
            }
            //可能是分配frameResampled的data的buffer
            ret = avcodec_fill_audio_frame(frameResampled, frameResampled->channels, (AVSampleFormat)frameResampled->format,
                (const uint8_t*)resampledBuffer.data(), resampledBuffSize, 0);
            if (ret < 0) {
                *errorCode = RET_M4A_GENERIC_ERROR;
                goto m4aDecEnd;
            }
            int outSampleNum;
            outSampleNum = swr_convert(convertCtx, frameResampled->data, frameResampled->nb_samples, (const uint8_t **)(frame->data), frame->nb_samples);
            int dataSize;
            dataSize = av_get_bytes_per_sample((AVSampleFormat)frameResampled->format);
            if (dataSize < 0) {
                *errorCode = RET_M4A_GENERIC_ERROR;
                /* This should not occur, checking just for paranoia */
                printf("[M4A DEC] Failed to calculate data size\n");
                break;
            }

            if (av_sample_fmt_is_planar((AVSampleFormat)frameResampled->format)) {
                for (i = 0; i < outSampleNum; i++) {
                    for (int ch = 0; ch < frameResampled->channels; ch++) {
                        outLen += dataSize;
                        if (outLen > outBufferSize) {
                            printf("[M4A DEC] Failed to calculate data size\n");
                            *errorCode = RET_M4A_OUT_BUFFER_FULL;
                            break;
                        }
                        memcpy(outData + outLen - dataSize, frameResampled->data[ch] + dataSize*i, dataSize);
                    }
                }
            } else {
                int resampledOutLen = outSampleNum * av_get_bytes_per_sample((AVSampleFormat)frameResampled->format) * frameResampled->channels;
                outLen += resampledOutLen;
                if (outLen > outBufferSize) {
                    printf("[M4A DEC] Failed to calculate data size\n");
                    *errorCode = RET_M4A_OUT_BUFFER_FULL;
                    break;
                }
                memcpy(outData + outLen - resampledOutLen, frameResampled->data[0], resampledOutLen);
            }
        }

        av_packet_unref(&packet);
    }

m4aDecEnd:
    if (frame) {
        av_frame_free(&frame);
        frame = NULL;
    }
    if (frameResampled) {
        av_frame_free(&frameResampled);
        frameResampled = NULL;
    }
    if (codecCtx) {
        avcodec_close(codecCtx);
        avcodec_free_context(&codecCtx);
        codecCtx = NULL;
    }
    if (ifmtCtx) {
        avformat_close_input(&ifmtCtx);
        ifmtCtx = NULL;
    }
    if (avioIn) {
        if (avioIn->buffer)
            av_free(avioIn->buffer);
        av_free(avioIn);
        avioIn = NULL;
    }
    //inBuffer should be release in avioIn
    inBuffer = NULL;
    if (inBuffer) {
        av_free(inBuffer);
        inBuffer = NULL;
    }
    if (convertCtx) {
        swr_free(&convertCtx);
    }

    return outLen;
}

void  CM4a_decoder::stop() {

}

void CM4a_decoder::start(int codec, int inputFrameSize, int sampleFrequency, int chans, int bps, int profile)
{

}
    



}
