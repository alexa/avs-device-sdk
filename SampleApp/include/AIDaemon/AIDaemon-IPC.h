#ifndef __AIDAEMON_IPC_H__
#define __AIDAEMON_IPC_H__

namespace AIDAEMON {
    const static std::string    SERVER_OBJECT = "/com/obigo/Nissan/AIDaemon";
    const static std::string    SERVER_INTERFACE = "com.obigo.Nissan.AIDaemon";

    static const std::string    IPC_METHODID("methodId");
    static const std::string    IPC_DATA("data");

    static const std::string    IPC_METHODID_NOTI_DIRECTIVE("NOTI_DIRECTIVE");
    static const std::string    IPC_METHODID_REQ_VR_START("REQ_VR_START");
    static const std::string    IPC_METHODID_REQ_VR_STOP("REQ_VR_STOP");

}



#endif /* ___AIDAEMON_IPC_H__ */
