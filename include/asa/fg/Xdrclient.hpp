#ifndef __XDR_CLIENT_HXX__
#define __XDR_CLIENT_HXX__

#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sys/time.h>
#include <thread>
#include <unistd.h>
#include <sstream>
#include <vector>

// Do not remove the include <boost/core/enable_if.hpp>. Need to avoid redefinition of structures by <simgear/props/props.hxx>
#include <boost/core/enable_if.hpp>
#include <simgear/props/props.hxx>
#include <simgear/math/SGMath.hxx>

#include "asa/fg/Processmsg.hpp"
#include "asa/fg/Utils.hpp"
#include "asa/fg/Mpmessages.hpp"

namespace asa::fg
{

    const int MIN_MP_PROTOCOL_VERSION = 1;
    const int MAX_MP_PROTOCOL_VERSION = 2;

    class XDRClient
    {
    public:
        XDRClient(const char *serverIp, int serverPort);

        ~XDRClient();

        static int toBuf(char *buffer,
                         const asa::fg::FGExternalMotionData &motionInfo,
                         int protocolToUse,
                         std::string model,
                         double timeAccel,
                         std::string callSign);

        bool sendBuffer(char *buffer, int len);

        bool send(
            const char *model,
            const double currentPosition[3],
            const float currentOrientation[3],
            const float linearVel[3],
            const float angularVel[3],
            const float linearAccel[3],
            const float angularAccel[3],
            const char *callsign,
            std::vector<asa::fg::FGPropertyData *> *properties,
            char *extra,
            int extra_len);

        void receive();
        bool recvData(MsgBuf *msg);

        static void populatePositionMsg(
            asa::fg::T_PositionMsg &positionMsg,
            asa::fg::FGExternalMotionData &motionInfo);

        static void populateMsgHdr(
            asa::fg::T_MsgHdr &msgHdr,
            asa::fg::FGExternalMotionData &motionInfo);

        static void populatePositionMsg(
            asa::fg::T_PositionMsg &positionMsg,
            const char *model,
            const double currentPosition[3],
            const float currentOrientation[3],
            const float linearVel[3],
            const float angularVel[3],
            const float linearAccel[3],
            const float angularAccel[3],
            double lag,
            double pad);

        static void populateMsgHdr(
            asa::fg::T_MsgHdr &msgHdr,
            uint32_t magic,
            uint32_t version,
            uint32_t msgId,
            uint32_t msgLen,
            uint32_t requestedRangeNmMin,
            uint32_t requestedRangeNmMax,
            uint16_t replyPort,
            const char *callsign);

    private:
        int serverSocket;
        struct sockaddr_in serverAddress;
        const char *serverIp;
        int serverPort;
    };

}

#endif /* __XDR_CLIENT_HXX__ */