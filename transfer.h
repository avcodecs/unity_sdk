#include"AudioBase.h"
namespace yymobile
{

class transfer{
public:
    
    transfer(AudioBase &inputInfo,AudioBase &outputInfo,int codec,int bps,int flag);

	~transfer();


    int Start();
    void Stop();
    void  operator()(int a);
    
private:
    AudioBase & m_inputInfo;
    AudioBase & m_outputInfo;
    int m_codec;
    int m_bps;
    int m_flag;
    
};

}