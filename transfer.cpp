#include "transfer.h"
namespace yymobile
{

    transfer::transfer(AudioBase& inputInfo, AudioBase& outputInfo, int codec, int bps,int flag):
        m_inputInfo(inputInfo),
        m_outputInfo(outputInfo),
        m_codec(codec),
        m_bps(bps),
        m_flag(flag){


    }
    

    transfer::~transfer(){
        Stop();
    }

    int transfer::Start(){
        
        return 0;
    }

    // int  transfer::operator()(AudioBase &inputInfo,AudioBase &outputInfo,int bps,int flag,int codec){
    //     m_inputInfo=inputInfo;
    //     m_outputInfo=outputInfo;
    //     m_bps=bps;
    //     m_flag=flag;
    //     m_codec=codec;
    //     return 0;
    // }

    void transfer::operator()(int a){




    }

}