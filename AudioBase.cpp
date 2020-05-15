#include "AudioBase.h"
namespace yymobile
{

    AudioBase::AudioBase(int SampleRate,int Channel,int ChannelLayout,int SampleFormat,std::vector<unsigned char> &Buffer):
        m_SampleRate(SampleRate), 
        m_Channel(Channel),
        m_ChannelLayout(ChannelLayout),
        m_SampleFormat(SampleFormat),
        m_buffer(Buffer)  // xuyao shiyong yinyong
    {
    }
    
    AudioBase::~AudioBase()
    {
    }
    
    // void transfer(AudioBase inputInfo,AudioBase outputInfo,int bps){

    // AudioBase& AudioBase::operator=(const AudioBase& inputInfo){
    //     this->m_Channel=inputInfo.m_Channel;
    //     this->m_ChannelLayout=inputInfo.m_ChannelLayout;
    //     this->m_SampleFormat=inputInfo.m_SampleFormat;
    //     this->m_SampleRate=inputInfo.m_SampleRate;
    //     this->m_buffer=inputInfo.m_buffer;
    //     return *this;
    // }

    // }






} // namespace yymobile
