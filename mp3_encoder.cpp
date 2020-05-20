#include "mp3_encoder.h"

namespace yymobile
{

    mp3_encoder::mp3_encoder() : codec(NULL), codecCtx(NULL), frame(NULL), pkt(NULL)
    {
        
    }

    mp3_encoder::~mp3_encoder()
    {
        Stop();
    }

    int mp3_encoder::Start(int insampleRate, int inchannel, AVSampleFormat insampleformat,int bps,int profile,int outsample,int outchannel,AVSampleFormat outsampleformat)
    {
        int bufferPos, ret;
        /* find the MP3 encoder */
        codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
        if (!codec)
        {
            printf("mp3_encoder avcodec_find_encoder fail");
            Stop();
            return -1;
        }

        codecCtx = avcodec_alloc_context3(codec);
        if (!codecCtx)
        {
            printf("mp3_encoder avcodec_alloc_context3 fail");
            Stop();
            return -1;
        }

        if (outchannel != 1 && outchannel != 2)
        {
            printf("mp3_encoder not support %d channel",outchannel);
            Stop();
            return -1;
        }

        /* put sample parameters */
        codecCtx->bit_rate = bps;
        codecCtx->sample_fmt = outsampleformat;
        codecCtx->sample_rate = outsample;
        codecCtx->channel_layout = (outchannel == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
        codecCtx->channels = av_get_channel_layout_nb_channels(codecCtx->channel_layout);

        /* open it */
        if (avcodec_open2(codecCtx, codec, NULL) < 0)
        {
            printf("mp3_encoder avcodec_open2 fail");
            Stop();
            return -1;
        }

        /* packet for holding encoded output */
        pkt = av_packet_alloc();
        if (!pkt)
        {
            printf("mp3_encoder av_packet_alloc fail");
            Stop();
            return -1;
        }

        /* frame containing input raw audio */
        frame = av_frame_alloc();
        if (!frame)
        {
            printf("mp3_encoder av_frame_alloc fail");
            Stop();
            return -1;
        }

        frame->nb_samples = codecCtx->frame_size;//maybe need resample if samplerate is not equal
        frame->format = insampleformat;
        frame->channel_layout = (inchannel == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
        frame->channels = inchannel;

        /* allocate the data buffers */
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0)
        {
            printf("mp3_encoder av_frame_get_buffer fail");
            Stop();
            return -1;
        }


    }

    void mp3_encoder::Stop()
    {
        if (frame)
        {
            av_frame_free(&frame);
            frame = NULL;
        }
        if (pkt)
        {
            av_packet_free(&pkt);
            pkt = NULL;
        }
        if (codecCtx)
        {
            avcodec_close(codecCtx);
            avcodec_free_context(&codecCtx);
            codecCtx = NULL;
        }
    }

    void mp3_encoder::encode_oneframe(AVCodecContext* codecCtx, AVFrame* frame, AVPacket* pkt,unsigned char *outData, int &curOutLen, int totalBufferSize)
    {
        int ret = 0;

        ret = avcodec_send_frame(codecCtx, frame);
        if (ret < 0)
        {
            return ;
        }

        while (ret >= 0)
        {
            ret = avcodec_receive_packet(codecCtx, pkt);
            if (ret < 0)
            {
                return ;
            }
            curOutLen += pkt->size;
            if (curOutLen > totalBufferSize)
            {
                printf("[MP3 ENC] Failed to calculate data size\n");
                return ;
            }
            memcpy(outData + curOutLen - pkt->size, pkt->data, pkt->size);
            av_packet_unref(pkt);
        }

        return ;
    }

    int mp3_encoder::Encode(const unsigned char* inputData, unsigned int len, unsigned char* outputData,int outBufferSize)
    {
        int outLen = 0;
        int bufferPos = 0;
        int ret=0;
        int frameBytesSize = codecCtx->frame_size * frame->channels * av_get_bytes_per_sample((AVSampleFormat)frame->format);
        while (1)
        {
            if (bufferPos + frameBytesSize > len)
            {
                break;
            }

            /* make sure the frame is writable -- makes a copy if the encoder kept a reference internally */
            ret = av_frame_make_writable(frame);
            if (ret < 0)
            {
                break;
            }

            memcpy(frame->data[0], inputData + bufferPos, frameBytesSize);

            bufferPos += frameBytesSize;

            encode_oneframe(codecCtx, frame, pkt,outputData, outLen, outBufferSize);
        }

        /* flush the encoder */
        encode_oneframe(codecCtx, NULL, pkt, outputData, outLen, outBufferSize);

        Stop();
        return outLen;
    }

} // namespace yymobile