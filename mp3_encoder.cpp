namespace yymobile
{

    mp3_encoder::mp3_encoder(int sampleFrequency, int channels, int bps, int bitrate):
        m_sampleFrequency(sampleFrequency), m_channels(channels), m_bps(bps), m_bitrate(bitrate), m_quality(quality), 
        m_lastQuality(quality), m_pSilkEnc(NULL), m_useDTX(0), m_complexity(0), m_EncCount(0),
        m_resampler(NULL), m_resampled_data(NULL), m_resample_data_size(0),mResamplerForSilk("silkcoder Adapter"){
        m_inputFrameSize = sampleFrequency* FRAME_LENGTH_MS  / 1000 * bps/8;
    }

    mp3_encoder::~mp3_encoder(){
        Stop();
    }

    int mp3_encoder::Start(){
        // if(m_bps != 16 )
        // 	return -1;

        // int encSizeBytes;
        // int ret = SKP_Silk_SDK_Get_Encoder_Size( &encSizeBytes );
        // if( ret ) {
        // 	return -1;
        //  }

        // m_pSilkEnc = malloc( encSizeBytes );

        // ret = SKP_Silk_SDK_InitEncoder( m_pSilkEnc, &m_encControl );
        // if( ret ) {
        // 	return -1;
        // }

        //#ifdef YYMOBILE_ANDROID
        // int coreCount = android_getCpuCount();
        // if( coreCount >= 4 )
        // {
        //     m_complexity=1;
        // 	LOGD("### silk encoder, use complexity : %d, core count :%d", m_complexity, coreCount);
        // }
        //#endif

        /* Set Encoder parameters */
        // m_encControl.API_sampleRate       = m_sampleFrequency;
        // m_encControl.maxInternalSampleRate  = 24000;
        // m_encControl.packetSize           = m_sampleFrequency * FRAME_LENGTH_MS/1000;
        // m_encControl.packetLossPercentage = 0;
        // m_encControl.useInBandFEC         = 0;
        // m_encControl.useDTX               = m_useDTX;
        // m_encControl.complexity           = m_complexity;
        // m_encControl.bitRate              = silkbps[m_quality];

        /*
        if(m_sampleFrequency != yymobile::kDefaultSampleRate)
        {
            LOGD("### silk encoder, need resample: %d -> %d", yymobile::kDefaultSampleRate, m_sampleFrequency);
            m_resampler = new webrtc::Resampler(yymobile::kDefaultSampleRate, m_sampleFrequency, webrtc::kResamplerSynchronous);
            m_resample_data_size = m_inputFrameSize * 2;
            m_resampled_data = (char*) malloc(m_resample_data_size);

            LOGD("### silk encoder: input_frame(%d), output_frame(%d), resample_frame(%d)",
                    m_inputFrameSize, (yymobile::kMaxEncodedBytesPerPacket + 2), m_resample_data_size);
        }
        */
        
        // if(m_sampleFrequency != yymobile::kSampleRate44K1)
        // {
        //     m_resample_data_size = m_inputFrameSize * 2;
        //     m_resampled_data = (char*) malloc(m_resample_data_size);
        // }
        // return (yymobile::kMaxEncodedBytesPerPacket + 2);
    }
}