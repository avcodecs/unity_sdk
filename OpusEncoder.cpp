
#include "OpusEncoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "logger.h"


#define MAX_BYTES_PER_FRAME     250 // Equals peak bitrate of 100 kbps
#define MAX_INPUT_FRAMES        4
#define FRAME_LENGTH_MS         20
static int opusbps[] = { 5000,  8000, 12000, 14000, 16000, 18000, 20000, 22000, 24000, 32000, 48000, 100000 };
#define ENC_APPLICATION 		OPUS_APPLICATION_AUDIO

COpusEncoder::COpusEncoder(int sampleFrequency, int channels, int bps, int quality)
	:m_sampleFrequency(sampleFrequency)
	, m_channels(channels)
	, m_bps(bps)
	, m_quality(quality)
	, m_useDTX(1)
	, m_complexity(10)
	, m_enc(NULL)
	, m_isInit(false)
	, m_convertCtx(NULL)
	, m_encodeSampleFrequency(48000)		//opus only support 16k/24k/48k)
	, m_resampleBuffer(NULL)
{
	m_inputFrameSize = sampleFrequency * FRAME_LENGTH_MS / 1000 * bps / 8;
}

COpusEncoder::~COpusEncoder() {
	Stop();
	m_isInit = false;
}


int COpusEncoder::Start() {
	int err;
	if (m_bps != 16)
		return -1;
		
	if (m_sampleFrequency == 16000 || m_sampleFrequency == 32000 || m_sampleFrequency == 48000)
		m_encodeSampleFrequency = m_sampleFrequency;
	else
		m_encodeSampleFrequency = 48000;
	
	m_enc = opus_encoder_create(m_encodeSampleFrequency, m_channels, ENC_APPLICATION, &err);
	if (err != OPUS_OK)
	{
		return -1;
	}
	
	opus_encoder_ctl(m_enc, OPUS_SET_BITRATE(opusbps[m_quality]));
	//opus_encoder_ctl(m_enc, OPUS_SET_BANDWIDTH(bandwidth));
	//opus_encoder_ctl(m_enc, OPUS_SET_VBR(use_vbr));
	//opus_encoder_ctl(m_enc, OPUS_SET_VBR_CONSTRAINT(cvbr));
	opus_encoder_ctl(m_enc, OPUS_SET_COMPLEXITY(m_complexity));		//default highest complexity
	//opus_encoder_ctl(m_enc, OPUS_SET_INBAND_FEC(use_inbandfec));
	//opus_encoder_ctl(m_enc, OPUS_SET_FORCE_CHANNELS(forcechannels));
	//opus_encoder_ctl(m_enc, OPUS_SET_DTX(use_dtx));
	//opus_encoder_ctl(m_enc, OPUS_SET_PACKET_LOSS_PERC(packet_loss_perc));
	
	//opus_encoder_ctl(m_enc, OPUS_GET_LOOKAHEAD(&skip));
	opus_encoder_ctl(m_enc, OPUS_SET_LSB_DEPTH(16));
	opus_encoder_ctl(m_enc, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_20_MS));
	
	printf("Encoding %ld Hz input at %.3f kb/s in %s with %d-sample frames.\n", (long)m_encodeSampleFrequency, opusbps[m_quality]*0.001, "auto", FRAME_LENGTH_MS * m_encodeSampleFrequency / 1000);

	avcodec_register_all();
	
	int encodeFrameSize = FRAME_LENGTH_MS * m_encodeSampleFrequency * m_channels * sizeof(short) / 1000;
	m_inputFrameSize = encodeFrameSize * m_sampleFrequency / m_encodeSampleFrequency;


	m_convertCtx = swr_alloc();
	uint64_t out_channel_layout = m_channels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
	uint64_t in_channel_layout = m_channels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
	m_convertCtx = swr_alloc_set_opts(m_convertCtx, out_channel_layout, AV_SAMPLE_FMT_S16, m_encodeSampleFrequency, 
		in_channel_layout, AV_SAMPLE_FMT_S16, m_sampleFrequency, 0, NULL);
	printf("out: %llu, %d, %d; in: %llu, %d, %d\n", out_channel_layout, AV_SAMPLE_FMT_S16, m_encodeSampleFrequency, in_channel_layout, AV_SAMPLE_FMT_S16, m_sampleFrequency);
	swr_init(m_convertCtx);


	m_resampleBuffer = new uint8_t[encodeFrameSize];
	uint8_t *test_cvt_in = new uint8_t[m_inputFrameSize];
	memset(test_cvt_in, 0, m_inputFrameSize);				//convert once for fill
	
	swr_convert(m_convertCtx, &m_resampleBuffer, FRAME_LENGTH_MS * m_encodeSampleFrequency / 1000, 
			(const uint8_t **)(&test_cvt_in), FRAME_LENGTH_MS * m_sampleFrequency / 1000);
	delete [] test_cvt_in;


	// Set Encoder parameters
	/*
	m_encControl.API_sampleRate = m_sampleFrequency;
	m_encControl.maxInternalSampleRate = 24000;
	m_encControl.packetSize = m_sampleFrequency * FRAME_LENGTH_MS / 1000;
	m_encControl.packetLossPercentage = 0;
	m_encControl.useInBandFEC = 0;
	m_encControl.useDTX = m_useDTX;
	m_encControl.complexity = m_complexity;
	m_encControl.bitRate = silkbps[m_quality];
	*/

	m_isInit = true;
	return m_inputFrameSize;


}


void COpusEncoder::Stop() {
	log(Info,"[COpusEncoder] Stop");
	if (m_enc != NULL)
	{
		log(Info,"[COpusEncoder] opus_encoder_destroy in");
    	opus_encoder_destroy(m_enc);
		m_enc = NULL;
	}

	if (m_resampleBuffer)
	{
		delete [] m_resampleBuffer;
		m_resampleBuffer = NULL;
	}

	if (m_convertCtx)
	{
		swr_free(&m_convertCtx);
		m_complexity = NULL;
	}
}



int COpusEncoder::Encode_empty_frame(const unsigned char* inputData, unsigned int length, unsigned char* outputData)
{
	if (!m_isInit || inputData == NULL || m_inputFrameSize > length) {
		return -1;
	}
	int outputLength = 0;
	const char* cur = (const char*)inputData;
	const short outLen = 0;
	unsigned short a = (0xFC << 8) | outLen;

	while (m_inputFrameSize <= length) {
		*(outputData + outputLength) = a & 0xFF;
		*(outputData + outputLength + 1) = (a & 0xFF00) >> 8;
		outputLength += 2;

		cur += m_inputFrameSize;
		length -= m_inputFrameSize;
	}

	return outputLength;
}



int COpusEncoder::Encode(const unsigned char* inputData, unsigned int length, unsigned char* outputData) {
	if (!m_isInit || inputData == NULL || m_inputFrameSize > length) {
		return -1;
	}
	int outputLength = 0;
	const char* cur = (const char*)inputData;
	short outLen = 0;

	while (m_inputFrameSize <= length) {
		outLen = MAX_BYTES_PER_FRAME * MAX_INPUT_FRAMES;
		short *in = (short *)cur;
		int encodeSample = FRAME_LENGTH_MS * m_encodeSampleFrequency / 1000;

		if (m_sampleFrequency != m_encodeSampleFrequency)
		{
			int inputSample = m_inputFrameSize / m_channels / sizeof(short);
			encodeSample = swr_convert(m_convertCtx, &m_resampleBuffer, encodeSample, (const uint8_t **)(&cur), inputSample);
			in = (short *)m_resampleBuffer;
		}
		else
		{
			// in = (short *)cur;
		}
		
		outLen = opus_encode(m_enc, in, encodeSample, m_outputBuffer, MAX_PAYLOAD_BYTES);
		
		//unsigned short a = (0xFC << 8) | outLen;
		//*(outputData + outputLength) = a & 0xFF;
		//*(outputData + outputLength + 1) = (a & 0xFF00) >> 8;
		//memcpy(outputData + outputLength + 2, m_outputBuffer, outLen);
		//outputLength += outLen + 2;
	
		memcpy(outputData + outputLength , m_outputBuffer, outLen);
		outputLength += outLen ;

		cur += m_inputFrameSize;
		length -= m_inputFrameSize;
	}

	return outputLength;
}
