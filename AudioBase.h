
#include<iostream>
#include<vector>

namespace yymobile
{






    class AudioBase // 
    {
    public:
        AudioBase(int SampleRate,int Channel,int ChannelLayout,int SampleFormat,std::vector<unsigned char> &Buffer);
        ~AudioBase();
        //AudioBase& operator=(const AudioBase&);

    private:
        /* data */
        int m_SampleRate;
        int m_Channel;
        uint64_t m_ChannelLayout;
        int m_SampleFormat;
        std::vector<unsigned char> &m_buffer;
    };



} // namespace yymobile
