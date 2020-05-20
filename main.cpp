#include<iostream>
#include "AudioBase.h"
#include "transfer.h"

extern "C"{
    #include "libavcodec/avcodec.h"
    #include "libavutil/channel_layout.h"
    #include "libavutil/common.h"
    #include "libavutil/frame.h"
    #include "libavutil/samplefmt.h"
    #include "libswresample/swresample.h"
    #include "libavutil/audio_fifo.h"
}

int main(){

    FILE *fp_open = NULL;
	FILE *fp_write = NULL;
	//16000 sampleRate 1 channel 16bit
	int inputSampleRate = 8000;
	fp_open = fopen("2.pcm", "rb");
	if (!fp_open) {
		printf("input file open error\n");
		exit(0);
	}
	fp_write = fopen("abc.opus", "wb");
	if (!fp_write) {
		printf("output file open error\n");
		exit(0);
	}

	std::vector<unsigned char> tmpInBuffer;
	std::vector<unsigned char> tmpOutBuffer;
	int inputLength = 0;
	int errorCode = 0;
	//unsigned long tStart = 0;
	//unsigned long tEnd = 0;
	//double audioLengthInSec = 0;
	fseek(fp_open, 0, SEEK_END);
	inputLength = ftell(fp_open);

	tmpInBuffer.resize(inputLength);

	fseek(fp_open, 0, SEEK_SET);
	fread(tmpInBuffer.data(), 1, inputLength, fp_open);


    yymobile::AudioBase inputInfo(8000,1,1,AV_SAMPLE_FMT_S16,tmpInBuffer);
    yymobile::AudioBase outputInfo(8000,1,1,AV_SAMPLE_FMT_S16,tmpOutBuffer);
    int bps=2400;

    yymobile::transfer ins_transfer(inputInfo,outputInfo,24000,1,errorCode);
    //inputInfo,outputInfo,24000,1,errorCode
    
    ins_transfer(7);
    
	fwrite(tmpOutBuffer.data(), 1, inputLength, fp_open);

    return 0;
}