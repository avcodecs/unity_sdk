
static int find_sample_index(int samplerate)
{
	int adts_sample_rates[] = { 96000,882000,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0 };
	int i;
	for (i = 0; i < 16; i++)
	{
		if (samplerate == adts_sample_rates[i])
			return i;
	}
	return 16 - 1;
}

#define AACHEADLEN 7
static int encodeAac(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt,
	unsigned char *outData, int &curOutLen, int totalBufferSize)
{
	int ret = 0;
	ret = avcodec_send_frame(ctx, frame);
	if (ret < 0) {
		return RET_AUDIO_ENCODE_ERROR;
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(ctx, pkt);
		if (ret < 0) {
			return RET_AUDIO_OK;
		}

		char bits[AACHEADLEN] = { 0 };
		int aac_frame_length = AACHEADLEN + pkt->size;
		int sample_index = find_sample_index(ctx->sample_rate);
		int channels = ctx->channels;
		if (channels == 8) {
			channels = 7;
		}

		bits[0] = 0xff;
		bits[1] = 0xf9;
		bits[2] = (ctx->profile << 6);
		bits[2] |= (sample_index << 2);
		bits[2] |= (channels >> 2);
		bits[3] |= ((channels << 6) & 0xC0);
		bits[3] |= (aac_frame_length >> 11);
		bits[4] = ((aac_frame_length >> 3) & 0xFF);
		bits[5] = ((aac_frame_length << 5) & 0xE0);
		bits[5] |= (0x7FF >> 6);

		curOutLen += aac_frame_length;
		if (curOutLen > totalBufferSize) {
			printf("[MP3 ENC] Failed to calculate data size\n");
			return RET_AUDIO_OUT_BUFFER_FULL;
		}
		memcpy(outData + curOutLen - pkt->size - AACHEADLEN, bits, AACHEADLEN);
		memcpy(outData + curOutLen - pkt->size, pkt->data, pkt->size);
		av_packet_unref(pkt);
	}

	return RET_AUDIO_OK;
}

static void pcmDeInterleav(short* leftChan, short* rightChan, short* sInData, int frameSize, int sInDataReadPos){
	for (int i = 0; i < frameSize; i++) {
		leftChan[i]  = sInData[sInDataReadPos + 2*i];
		rightChan[i] = sInData[sInDataReadPos + 2*i + 1];
	}	
}

static int add_samples_to_fifo(AVAudioFifo *fifo,
	uint8_t **converted_input_samples,
	const int frame_size)
{
	int error;

	/* Make the FIFO as large as it needs to be to hold both,
	 * the old and the new samples. */
	if ((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frame_size)) < 0) {
		fprintf(stderr, "Could not reallocate FIFO\n");
		return error;
	}

	/* Store the new samples in the FIFO buffer. */
	if (av_audio_fifo_write(fifo, (void **)converted_input_samples,
		frame_size) < frame_size) {
		fprintf(stderr, "Could not write data to FIFO\n");
		return AVERROR_EXIT;
	}
	return 0;
}

static int HelloAudioInternalEncode(int inSampleRate, int inChannel, int bps, unsigned char *inData, int inLength,
					unsigned char *outData, int outBufferSize, int *errorCode, int encType, int aacEncProfile)
{
	const AVCodec *codec;
	AVCodecContext *codecCtx = NULL;
	AVFrame *inframe = NULL;
	AVFrame *encframe = NULL;
	AVPacket *pkt  = NULL;
	struct SwrContext *swr_ctx;
	uint8_t **dst_data = NULL;
	int dst_linesize;
	int bufferPos = 0, ret, inframeBytesSize;
	int outLen = 0;
	AVAudioFifo *fifo = NULL;
	int aacEncChannel = (aacEncProfile == FF_PROFILE_AAC_HE_V2) ? 2 : 1;//ps用双通道其他用单通道
	int aacEncSampleRate = inSampleRate;
	int dst_nb_samples = 0, max_dst_nb_samples = 0;
	uint64_t out_channel_layout = AV_CH_LAYOUT_MONO;
	uint64_t in_channel_layout = AV_CH_LAYOUT_MONO;

#ifdef WIN32
	if (firstInit) {
		av_register_all();
		firstInit = false;
	}
#endif

	swr_ctx = swr_alloc();
	if (!swr_ctx) {
		printf("[AudioInternal ENC] Could not allocate resampler context\n");
		*errorCode = RET_AUDIO_GENERIC_ERROR;
		goto audioInternalEncodeEnd;
	}

	out_channel_layout = aacEncChannel == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
	in_channel_layout = inChannel == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
	swr_ctx = swr_alloc_set_opts(NULL, out_channel_layout, AV_SAMPLE_FMT_FLTP, aacEncSampleRate, \
		in_channel_layout, AV_SAMPLE_FMT_S16, inSampleRate, 0, NULL);

	if ((ret = swr_init(swr_ctx)) < 0) {
		printf("[AudioInternal ENC] Failed to initialize the resampling context\n");
		*errorCode = RET_AUDIO_GENERIC_ERROR;
		goto audioInternalEncodeEnd;
	}
	
	/* find the audio encoder */
	codec = avcodec_find_encoder(AVCodecID(encType));
	if (!codec) {
		printf("[AudioInternal ENC] not support encType %d\n", encType);
		*errorCode = RET_AUDIO_GENERIC_ERROR;
		goto audioInternalEncodeEnd;
	}

	codecCtx = avcodec_alloc_context3(codec);
	if (!codecCtx) {
		printf("[AudioInternal ENC] not support codec");
		*errorCode = RET_AUDIO_GENERIC_ERROR;
		goto audioInternalEncodeEnd;
	}

	if (inChannel != 1 && inChannel != 2) {
		printf("[AudioInternal ENC] not support channel %d\n", inChannel);
		*errorCode = RET_AUDIO_GENERIC_ERROR;
		goto audioInternalEncodeEnd;
	}

	/* put sample parameters */
	codecCtx->codec_type     = AVMEDIA_TYPE_AUDIO;
	codecCtx->bit_rate       = bps;
	codecCtx->profile        = aacEncProfile;
	codecCtx->sample_fmt     = AV_SAMPLE_FMT_FLTP;
	codecCtx->sample_rate    = aacEncSampleRate;
	codecCtx->channel_layout = out_channel_layout;
	codecCtx->channels       = av_get_channel_layout_nb_channels(codecCtx->channel_layout);
	codecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	/* open it */
	if (avcodec_open2(codecCtx, codec, NULL) < 0) {
		printf("[AudioInternal ENC] not support open2\n");
		*errorCode = RET_AUDIO_GENERIC_ERROR;
		goto audioInternalEncodeEnd;
	}

	/* packet for holding encoded output */
	pkt = av_packet_alloc();
	if (!pkt) {
		*errorCode = RET_AUDIO_GENERIC_ERROR;
		goto audioInternalEncodeEnd;
	}

	/* frame containing input raw audio */
	encframe = av_frame_alloc();
	if (!encframe) {
		*errorCode = RET_AUDIO_GENERIC_ERROR;
		goto audioInternalEncodeEnd;
	}

	encframe->sample_rate    = codecCtx->sample_rate;
	encframe->nb_samples     = codecCtx->frame_size;
	encframe->format         = codecCtx->sample_fmt;
	encframe->channel_layout = codecCtx->channel_layout;
	encframe->channels       = codecCtx->channels;

	/* allocate the data buffers */
	ret = av_frame_get_buffer(encframe, 0);
	if (ret < 0) {
		*errorCode = RET_AUDIO_GENERIC_ERROR;
		goto audioInternalEncodeEnd;
	}

	/* frame containing input raw audio */
	inframe = av_frame_alloc();
	if (!inframe) {
		*errorCode = RET_AUDIO_GENERIC_ERROR;
		goto audioInternalEncodeEnd;
	}

	inframe->sample_rate    = inSampleRate;
	inframe->nb_samples     = encframe->nb_samples;
	inframe->format         = AV_SAMPLE_FMT_S16;
	inframe->channel_layout = in_channel_layout;
	inframe->channels       = av_get_channel_layout_nb_channels(inframe->channel_layout);

	/* allocate the data buffers */
	ret = av_frame_get_buffer(inframe, 0);
	if (ret < 0) {
		*errorCode = RET_AUDIO_GENERIC_ERROR;
		goto audioInternalEncodeEnd;
	}

	max_dst_nb_samples = av_rescale_rnd(inframe->nb_samples, encframe->sample_rate, inframe->sample_rate, AV_ROUND_UP);
	dst_nb_samples = max_dst_nb_samples;
	ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, encframe->channels,
		dst_nb_samples, (AVSampleFormat)encframe->format, 0);
	if (ret < 0) {
		*errorCode = RET_AUDIO_GENERIC_ERROR;
		goto audioInternalEncodeEnd;
	}

	if (!(fifo = av_audio_fifo_alloc(codecCtx->sample_fmt, codecCtx->channels, 1))) {
		*errorCode = RET_AUDIO_GENERIC_ERROR;
		goto audioInternalEncodeEnd;
	}

	inframeBytesSize = inframe->nb_samples * sizeof(AV_SAMPLE_FMT_S16) * inChannel;
	while (1) {
		if (bufferPos + inframeBytesSize > inLength) {
			break;
		}

		while (av_audio_fifo_size(fifo) < codecCtx->frame_size) {
			dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, inframe->sample_rate) +
				inframe->nb_samples, encframe->sample_rate, inframe->sample_rate, AV_ROUND_UP);
			if (dst_nb_samples > max_dst_nb_samples) {
				if (dst_data) {
					av_freep(&dst_data[0]);
				}
				ret = av_samples_alloc(dst_data, &dst_linesize, encframe->channels,
					dst_nb_samples, (AVSampleFormat)encframe->format, 1);
				if (ret < 0)
					break;
				max_dst_nb_samples = dst_nb_samples;
			}

			/* make sure the frame is writable -- makes a copy if the encoder
			* kept a reference internally */
			ret = av_frame_make_writable(inframe);
			if (ret < 0) {
				*errorCode = RET_AUDIO_ENCODE_ERROR;
				break;
			}

			memcpy(inframe->data[0], inData + bufferPos, inframe->nb_samples * sizeof(short) * inChannel);
			bufferPos += inframe->nb_samples * sizeof(short) * inChannel;

			ret = swr_convert(swr_ctx, dst_data, dst_nb_samples, (const uint8_t **)inframe->data, inframe->nb_samples);
			if (ret < 0) {
				*errorCode = RET_AUDIO_ENCODE_ERROR;
				break;
			}

			if (add_samples_to_fifo(fifo, dst_data, ret)) {
				*errorCode = RET_AUDIO_ENCODE_ERROR;
				break;
			}
		}

		while (av_audio_fifo_size(fifo) >= codecCtx->frame_size) {
			if (av_frame_make_writable(encframe) < 0) {
				*errorCode = RET_AUDIO_ENCODE_ERROR;
				break;
			}

			if (av_audio_fifo_read(fifo, (void **)encframe->data, codecCtx->frame_size) < codecCtx->frame_size) {
				*errorCode = RET_AUDIO_ENCODE_ERROR;
				break;
			}

			ret = encodeAac(codecCtx, encframe, pkt, outData, outLen, outBufferSize);
			*errorCode = ret;
		}
	}

	/* flush the encoder */
	ret = encodeAac(codecCtx, NULL, pkt, outData, outLen, outBufferSize);
	*errorCode = ret;

audioInternalEncodeEnd:
	if (dst_data) {
		av_freep(&dst_data[0]);
		av_freep(&dst_data);
		dst_data = NULL;
	}

	if (encframe) {
		av_frame_free(&encframe);
		encframe = NULL;
	}
	if (inframe) {
		av_frame_free(&inframe);
		inframe = NULL;
	}
	if (pkt) {
		av_packet_free(&pkt);
		pkt = NULL;
	}
	if (codecCtx) {
		avcodec_close(codecCtx);
		avcodec_free_context(&codecCtx);
		codecCtx = NULL;
	}

	if (fifo) {
		av_audio_fifo_free(fifo);
		fifo = NULL;
	}

	swr_free(&swr_ctx);
	swr_ctx = NULL;
	
	return outLen;
}




namespace yymobile
{





}