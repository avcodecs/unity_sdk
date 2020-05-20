
#include "OpusDecoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>


COpusDecoder::COpusDecoder(unsigned int inputFrameSize, int sampleFrequency, int channels, int bps) 
	:m_sampleFrequency(sampleFrequency)
	, m_channels(channels)
	, m_bps(bps)
	, m_inputFrameSize(inputFrameSize)
	, m_codec(NULL)
	, m_c(NULL)
	, m_lastFrameOutSize(1920)		//20ms 48k 16bit
	, m_outputFrame(NULL)
	, m_pkt(NULL)
	, m_convertCtx(NULL)
	, m_decodedSampleFrequency(48000)
	, m_decodedChannels(2)
	, m_decodedFormat(AV_SAMPLE_FMT_FLTP)
{
}

COpusDecoder::~COpusDecoder()
{
	Stop();
}


bool COpusDecoder::Start()
{

	avcodec_register_all();
	m_codec = avcodec_find_decoder(AV_CODEC_ID_OPUS);
	if (!m_codec) {
		return false;
	}

	m_c = avcodec_alloc_context3(m_codec);
	if (!m_c) {
		return false;
	}

	m_outputFrame = av_frame_alloc();
	if (!m_outputFrame) {
		return false;
	}

	m_pkt = av_packet_alloc();
	if (!m_pkt) {
		return false;
	}
	av_init_packet(m_pkt);

	m_convertCtx = swr_alloc();
	uint64_t out_channel_layout = m_channels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
	uint64_t in_channel_layout = m_decodedChannels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
		
	m_convertCtx = swr_alloc_set_opts(m_convertCtx, out_channel_layout, AV_SAMPLE_FMT_S16, m_sampleFrequency, \
		in_channel_layout, m_decodedFormat, m_decodedSampleFrequency, 0, NULL);
	swr_init(m_convertCtx);

	if (avcodec_open2(m_c, m_codec, NULL) < 0) {
		return false;
	}

	return true;
}


int COpusDecoder::Recover(int framenum, unsigned char* outputData)
{
	int outputLength = m_lastFrameOutSize * framenum;
	memset(outputData, 0, outputLength);
	return outputLength;
}

static inline int FrameLength(const unsigned char* audio, int len) {
	if (len < 2)
		return -1;

	short header = (*(short*)audio);
	if ((header & 0xfc00) != 0xfc00)
		return -1;

	return (header & 0x3ff);
}


int COpusDecoder::Decode(const unsigned char* inputData, unsigned int length, unsigned char* outputData)
{

	int outputLength = 0;
	const unsigned char* cur = inputData;
	const unsigned char* end = inputData + length;
	int isGotData = 0;
	const int MAX_AUDIO_FRAME_SIZE = 8192;

	while (cur < end)
	{
		int nFrameLen = length;//FrameLength(cur, length);
		//if (nFrameLen < 0 || (nFrameLen + 2) > length) {
		if (nFrameLen < 0) {
			break;
		}

		if (nFrameLen == 0) {
			memset(outputData + outputLength, 0, m_lastFrameOutSize); //empty frame
			outputLength += m_lastFrameOutSize;
		}
		else {
			short outlen = 0;
			m_pkt->data = (uint8_t*)cur;
			m_pkt->size = nFrameLen;
			int ret = avcodec_decode_audio4(m_c, m_outputFrame, &isGotData, m_pkt);
			if (m_outputFrame->channels != m_decodedChannels
				|| m_outputFrame->sample_rate != m_decodedSampleFrequency
				|| m_outputFrame->format != m_decodedFormat)
			{
				m_decodedChannels = m_outputFrame->channels;
				m_decodedSampleFrequency = m_outputFrame->sample_rate;
				m_decodedFormat = (AVSampleFormat)m_outputFrame->format;
				uint64_t out_channel_layout = m_channels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
				uint64_t in_channel_layout = m_decodedChannels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
				m_convertCtx = swr_alloc_set_opts(m_convertCtx, out_channel_layout, AV_SAMPLE_FMT_S16, m_sampleFrequency, \
					in_channel_layout, AV_SAMPLE_FMT_FLTP, m_decodedSampleFrequency, 0, NULL);
				swr_init(m_convertCtx);
			}

			uint8_t *pDataOut = (uint8_t *)(outputData + outputLength);
			
			ret = swr_convert(m_convertCtx, &pDataOut, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)m_outputFrame->data, m_outputFrame->nb_samples);

			int sampleSize = ret >= 0 ? ret * m_channels : 0;

			outputLength += sampleSize * 2;
			m_lastFrameOutSize = sampleSize * 2;
		}

		cur += ( nFrameLen);
		length -= (nFrameLen);
	}

	return outputLength;
}


void COpusDecoder::Stop()
{
	if (m_c)
	{
		avcodec_free_context(&m_c);
		m_c = NULL;
	}
	if (m_outputFrame)
	{
		av_frame_free(&m_outputFrame);
		m_outputFrame = NULL;
	}
	if (m_pkt)
	{
		av_packet_from_data(m_pkt, NULL, 0);
		av_packet_free(&m_pkt);
		m_pkt = NULL;
	}
	if (m_convertCtx)
	{
		swr_free(&m_convertCtx);
		m_convertCtx = NULL;
	}
}
