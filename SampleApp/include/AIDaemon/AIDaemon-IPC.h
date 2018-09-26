#ifndef __AIDAEMON_IPC_H__
#define __AIDAEMON_IPC_H__

namespace AIDAEMON {
    static const int            NULL_DATA=-1;    

    const static std::string    SERVER_OBJECT = "/com/obigo/Nissan/AIDaemon";
    const static std::string    SERVER_INTERFACE = "com.obigo.Nissan.AIDaemon";

    static const std::string    IPC_METHODID("methodId");
    static const std::string    IPC_DATA("data");

    static const std::string    METHODID_NOTI_DIRECTIVE("NOTI_DIRECTIVE");
    static const std::string    METHODID_NOTI_VR_STATE("NOTI_VR_STATE");
        static const int        NOTI_VR_STATE_IDLE=1;
        static const int        NOTI_VR_STATE_LISTENING=2;
        static const int        NOTI_VR_STATE_THINKING=3;
    static const std::string    METHODID_NOTI_TTS_START("NOTI_TTS_START");
    static const std::string    METHODID_NOTI_TTS_FINISH("NOTI_TTS_END");                
    static const std::string    METHODID_REQ_VR_START("REQ_VR_START");
    static const std::string    METHODID_REQ_EVENT_CANCEL("REQ_EVENT_CANCEL");

}



#endif /* ___AIDAEMON_IPC_H__ */
