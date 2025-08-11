#include "asa/fg/Xdrclient.hpp"
#include "asa/core/base/Log.hpp"
#include <poll.h>

#include <iostream>
#include <fstream>  
#include <chrono>
#include <string>

namespace asa::fg
{

    XDRClient::XDRClient(const char *serverIp, int serverPort)
    {

        // Attemps to create a UDP socket
        serverSocket = socket(AF_INET, SOCK_DGRAM, 0);

        if (serverSocket == -1)
        {
            ASA_LOG(error) << "[ERROR] Failed to create socket at " << __FUNCTION__ << ": " << strerror(errno) << " (" << errno << ") "
                      << "\n";
            std::exit(1);
        }

        // Set up the server address structure
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(serverPort);

        if (inet_pton(AF_INET, serverIp, &serverAddress.sin_addr) <= 0)
        {
            ASA_LOG(error) << "[ERROR] Invalid address or address not supported at " << __FUNCTION__ << ": " << strerror(errno) << " (" << errno << ") "
                      << "\n";
            close(serverSocket);
            std::exit(1);
        }
        // struct timeval tv;

        // tv.tv_sec = 0;
        // tv.tv_usec = 100;
        // int rc = setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        // if (rc < 0)
        // {
        //     ASA_LOG(error) << "[ERROR] error setting timeout " << "\n";
        // }
    }

    XDRClient::~XDRClient()
    {

        // Close the socket if it was opened
        if (serverSocket != -1)
        {
            close(serverSocket);
        }
    }

    int XDRClient::toBuf(char *buffer,
                         const FGExternalMotionData &motionInfo,
                         int protocolToUse,
                         std::string model,
                         double timeAccel,
                         std::string callSign)
    {

        // TODO: mover esse codigo para outra parte, pois so precisa ser rodado 1x
        PropertyDefinitionMap mPropertyDefinition;

        for (unsigned int i = 0; i < numProperties; i++)
        {
            mPropertyDefinition[sIdPropertyList[i].id] = &(sIdPropertyList[i]);
        }

        // Codigo copiado de multiplayermgr.cxx (flightgerar)
        // void  FGMultiplayMgr::SendMyPosition(const FGExternalMotionData &motionInfo)

        if (!isSane(motionInfo))
        {
            // Current local data is invalid (NaN), so stop MP transmission.
            // => Be nice to older FG versions (no NaN checks) and don't waste bandwidth.
            // SG_LOG(SG_NETWORK, SG_ALERT, "FGMultiplayMgr::SendMyPosition - "
            //<< "Local data is invalid (NaN). Data not transmitted.");
            ASA_LOG(error) << "[ERROR] Invalid motionInfo" << std::endl;
            return 0;
        }

        static MsgBuf msgBuf;
        static unsigned msgLen = 0;
        T_PositionMsg *PosMsg = msgBuf.posMsg();

        /*
         * This is to provide a level of compatibility with the new V2 packets.
         * By setting padding it will force older clients to use verify properties which will
         * bail out if there are any unknown props
         * MP2017(V2) (for V1 clients) will always have an unknown property because V2 transmits
         * the protocol version as the very first property as a shortint.
         */
        if (protocolToUse > 1)
        {
            PosMsg->pad = XDR_encode_int32(V2_PAD_MAGIC);
        }
        else
        {
            PosMsg->pad = 0;
        }

        strncpy(PosMsg->Model, model.c_str(), MAX_MODEL_NAME_LEN);
        PosMsg->Model[MAX_MODEL_NAME_LEN - 1] = '\0';

        PosMsg->time = XDR_encode_double(motionInfo.time);
        PosMsg->lag = XDR_encode_double(motionInfo.lag);
        for (unsigned i = 0; i < 3; ++i)
        {
            PosMsg->position[i] = XDR_encode_double(motionInfo.position(i));
        }
        SGVec3f angleAxis;
        motionInfo.orientation.getAngleAxis(angleAxis);
        for (unsigned i = 0; i < 3; ++i)
        {
            PosMsg->orientation[i] = XDR_encode_float(angleAxis(i));
        }

        // including speed up time in velocity and acceleration

        // TODO: fazer tudo isso num unico for
        for (unsigned i = 0; i < 3; ++i)
        {
            PosMsg->linearVel[i] = XDR_encode_float(motionInfo.linearVel(i) * timeAccel);
        }
        for (unsigned i = 0; i < 3; ++i)
        {
            PosMsg->angularVel[i] = XDR_encode_float(motionInfo.angularVel(i) * timeAccel);
        }
        for (unsigned i = 0; i < 3; ++i)
        {
            PosMsg->linearAccel[i] = XDR_encode_float(motionInfo.linearAccel(i) * timeAccel * timeAccel);
        }
        for (unsigned i = 0; i < 3; ++i)
        {
            PosMsg->angularAccel[i] = XDR_encode_float(motionInfo.angularAccel(i) * timeAccel * timeAccel);
        }

        xdr_data_t *ptr = msgBuf.properties();

        xdr_data_t *msgEnd = msgBuf.propsEnd();

        struct BoolArrayBuffer boolBuffer[MAX_BOOL_BUFFERS];
        memset(&boolBuffer, 0, sizeof(boolBuffer));

        for (int partition = 1; partition <= protocolToUse; partition++)
        {
            std::vector<FGPropertyData *>::const_iterator it = motionInfo.properties.begin();

            while (it != motionInfo.properties.end())
            {
                const struct IdPropertyList *propDef = mPropertyDefinition[(*it)->id];

                if ((*it)->id == 811)
                {
                    std::cout << std::endl;
                }

                /*
                 * Excludes the 2017.2 property for the protocol version from V1 packets.
                 */
                if (protocolToUse == 1 && propDef->version == V2_PROP_ID_PROTOCOL)
                {
                    ++it;
                    continue;
                }

                /*
                 * 2017.2 partitions the buffer sent into protocol versions. Originally this was intended to allow
                 * compatability with older clients; however this will only work in the future or with support from fgms
                 * - so if a future version adds more properties to the protocol these can be transmitted in a third partition
                 *   that will be ignored by older clients (such as 2017.2).
                 */
                if ((propDef->version & 0xffff) == partition || (propDef->version & 0xffff) > protocolToUse)
                {
                    if (ptr + 2 >= msgEnd)
                    {
                        ASA_LOG(warning) << "Multiplayer packet truncated prop id: " << (*it)->id << ": " << propDef->name;
                        break;
                    }

                    // First element is the ID. Write it out when we know we have room for
                    // the whole property.
                    xdr_data_t id = XDR_encode_uint32((*it)->id);

                    /*
                     * 2017.2 protocol has the ability to transmit as a different type (to save space), so
                     * process this when using this protocol (protocolVersion 2) or later
                     */
                    int transmit_type = (*it)->type;

                    if (propDef->TransmitAs != TT_ASIS && protocolToUse > 1)
                    {
                        transmit_type = propDef->TransmitAs;
                    }
                    else if (propDef->TransmitAs == TT_BOOLARRAY)
                        transmit_type = propDef->TransmitAs;

                    // ASA_LOG(debug) << "[SEND] pt " << partition << ": buf[" << (ptr - data) * sizeof(*ptr)
                    //                << "] id=" << (*it)->id << " type " << transmit_type;

                    if (propDef->encode_for_transmit && protocolToUse > 1)
                    {
                        ptr = (*propDef->encode_for_transmit)(propDef, ptr, (*it));
                    }
                    else
                    {
                        // The actual data representation depends on the type
                        switch (transmit_type)
                        {
                        case TT_NOSEND:
                            break;
                        case TT_SHORTINT:
                        {
                            *ptr++ = XDR_encode_shortints32((*it)->id, (*it)->int_value);
                            break;
                        }
                        case TT_SHORT_FLOAT_1:
                        {
                            short value = get_scaled_short((*it)->float_value, 10.0);
                            *ptr++ = XDR_encode_shortints32((*it)->id, value);
                            break;
                        }
                        case TT_SHORT_FLOAT_2:
                        {
                            short value = get_scaled_short((*it)->float_value, 100.0);
                            *ptr++ = XDR_encode_shortints32((*it)->id, value);
                            break;
                        }
                        case TT_SHORT_FLOAT_3:
                        {
                            short value = get_scaled_short((*it)->float_value, 1000.0);
                            *ptr++ = XDR_encode_shortints32((*it)->id, value);
                            break;
                        }
                        case TT_SHORT_FLOAT_4:
                        {
                            short value = get_scaled_short((*it)->float_value, 10000.0);
                            *ptr++ = XDR_encode_shortints32((*it)->id, value);
                            break;
                        }

                        case TT_SHORT_FLOAT_NORM:
                        {
                            short value = get_scaled_short((*it)->float_value, 32767.0);
                            *ptr++ = XDR_encode_shortints32((*it)->id, value);
                            break;
                        }
                        case TT_BOOLARRAY:
                        {
                            struct BoolArrayBuffer *boolBuf = nullptr;
                            if ((*it)->id >= BOOLARRAY_START_ID && (*it)->id <= BOOLARRAY_END_ID + BOOLARRAY_BLOCKSIZE)
                            {
                                int buffer_block = ((*it)->id - BOOLARRAY_BASE_1) / BOOLARRAY_BLOCKSIZE;
                                boolBuf = &boolBuffer[buffer_block];
                                boolBuf->propertyId = BOOLARRAY_START_ID + buffer_block * BOOLARRAY_BLOCKSIZE;
                            }
                            if (boolBuf)
                            {
                                int bitidx = (*it)->id - boolBuf->propertyId;
                                if ((*it)->int_value)
                                    boolBuf->boolValue |= 1 << bitidx;
                            }
                            break;
                        }
                        case simgear::props::INT:
                        case simgear::props::BOOL:
                        case simgear::props::LONG:
                            *ptr++ = id;
                            *ptr++ = XDR_encode_uint32((*it)->int_value);
                            break;
                        case simgear::props::FLOAT:
                        case simgear::props::DOUBLE:
                            *ptr++ = id;
                            *ptr++ = XDR_encode_float((*it)->float_value);
                            break;
                        case simgear::props::STRING:
                        case simgear::props::UNSPECIFIED:
                        {
                            if (protocolToUse > 1)
                            {
                                // New string encoding:
                                // xdr[0] : ID length packed into 32 bit containing two shorts.
                                // xdr[1..len/4] The string itself (char[length])
                                const char *lcharptr = (*it)->string_value;

                                if (lcharptr != 0)
                                {
                                    uint32_t len = strlen(lcharptr);

                                    if (len >= MAX_TEXT_SIZE)
                                    {
                                        len = MAX_TEXT_SIZE - 1;
                                        ASA_LOG(warning) << "Multiplayer property truncated at MAX_TEXT_SIZE in string " << (*it)->id;
                                    }

                                    char *encodeStart = (char *)ptr;
                                    char *msgEndbyte = (char *)msgEnd;

                                    if (encodeStart + 2 + len >= msgEndbyte)
                                    {
                                        ASA_LOG(warning) << "Multiplayer property not sent (no room) string " << (*it)->id;
                                        goto escape;
                                    }

                                    *ptr++ = XDR_encode_shortints32((*it)->id, len);
                                    encodeStart = (char *)ptr;
                                    if (len != 0)
                                    {
                                        int lcount = 0;
                                        while (*lcharptr && (lcount < MAX_TEXT_SIZE))
                                        {
                                            if (encodeStart + 2 >= msgEndbyte)
                                            {
                                                ASA_LOG(warning) << "Multiplayer packet truncated in string " << (*it)->id << " lcount " << lcount;
                                                break;
                                            }
                                            *encodeStart++ = *lcharptr++;
                                            lcount++;
                                        }
                                    }
                                    ptr = (xdr_data_t *)encodeStart;
                                }
                                else
                                {
                                    // empty string, just send the id and a zero length
                                    *ptr++ = id;
                                    *ptr++ = XDR_encode_uint32(0);
                                }
                            }
                            else
                            {

                                // String is complicated. It consists of
                                // The length of the string
                                // The string itself
                                // Padding to the nearest 4-bytes.
                                const char *lcharptr = (*it)->string_value;

                                if (lcharptr != 0)
                                {
                                    // Add the length
                                    ////cout << "String length: " << strlen(lcharptr) << "\n";
                                    uint32_t len = strlen(lcharptr);
                                    if (len >= MAX_TEXT_SIZE)
                                    {
                                        len = MAX_TEXT_SIZE - 1;
                                        // SG_LOG(SG_NETWORK, SG_ALERT, "Multiplayer property truncated at MAX_TEXT_SIZE in string " << (*it)->id);
                                    }

                                    // XXX This should not be using 4 bytes per character!
                                    // If there's not enough room for this property, drop it
                                    // on the floor.
                                    if (ptr + 2 + ((len + 3) & ~3) >= msgEnd)
                                    {
                                        // SG_LOG(SG_NETWORK, SG_ALERT, "Multiplayer property not sent (no room) string " << (*it)->id);
                                        goto escape;
                                    }
                                    // cout << "String length unint32: " << len << "\n";
                                    *ptr++ = id;
                                    *ptr++ = XDR_encode_uint32(len);
                                    if (len != 0)
                                    {
                                        // Now the text itself
                                        // XXX This should not be using 4 bytes per character!
                                        int lcount = 0;
                                        while ((*lcharptr != '\0') && (lcount < MAX_TEXT_SIZE))
                                        {
                                            if (ptr + 2 >= msgEnd)
                                            {
                                                ASA_LOG(warning) << "Multiplayer packet truncated in string " << (*it)->id << " lcount " << lcount;
                                                break;
                                            }
                                            *ptr++ = XDR_encode_int8(*lcharptr);
                                            lcharptr++;
                                            lcount++;
                                        }
                                        // Now pad if required
                                        while ((lcount % 4) != 0)
                                        {
                                            if (ptr + 2 >= msgEnd)
                                            {
                                                ASA_LOG(warning) << "Multiplayer packet truncated in string " << (*it)->id << " lcount " << lcount;
                                                break;
                                            }
                                            *ptr++ = XDR_encode_int8(0);
                                            lcount++;
                                        }
                                    }
                                }
                                else
                                {
                                    // Nothing to encode
                                    *ptr++ = id;
                                    *ptr++ = XDR_encode_uint32(0);
                                }
                            }
                        }
                        break;

                        default:
                            *ptr++ = id;
                            *ptr++ = XDR_encode_float((*it)->float_value);
                            ;
                            break;
                        }
                    }
                }
                ++it;
            }
        }
    escape:

        /*
         * Send the boolean arrays (if present) as single 32bit integers.
         */
        for (int boolIdx = 0; boolIdx < MAX_BOOL_BUFFERS; boolIdx++)
        {
            if (boolBuffer[boolIdx].propertyId)
            {
                if (ptr + 2 >= msgEnd)
                {
                    ASA_LOG(warning) << "Multiplayer packet truncated prop id: " << boolBuffer[boolIdx].propertyId << ": multiplay/generic/bools[" << boolIdx * 30 << "]";
                }
                *ptr++ = XDR_encode_int32(boolBuffer[boolIdx].propertyId);
                *ptr++ = XDR_encode_int32(boolBuffer[boolIdx].boolValue);
            }
        }

        msgLen = reinterpret_cast<char *>(ptr) - msgBuf.Msg;
        FillMsgHdr(msgBuf.msgHdr(), POS_DATA_ID, msgLen, callSign);

        /*
         * Informational:
         * Save the last packet length sent, and
         * if the property is set then dump the packet length to the console.
         * ----------------------------
         * This should be sufficient for rudimentary debugging (in order of useful ness)
         *  1. loopback your own craft. fantastic for resolving animations and property transmission issues.
         *  2. see what properties are being sent
         *  3. see how much space it takes up
         *  4. dump the packet as it goes out
         *  5. dump incoming packets
         */

        // ASA_LOG(debug) << "[SEND] Packet len " << msgLen;

        // ASA_LOG(debug) << data << (ptr - data) * sizeof(*ptr);
        /*
         * simple loopback of ourselves - to enable easy MP debug for model developers; see (1) above
         */

        memcpy(buffer, msgBuf.Msg, msgLen);
        return msgLen;

        // if (msgLen > 0)
        // {

        //     if (sendto(serverSocket, msgBuf.Msg, msgLen, 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
        //     {
        //         ASA_LOG(error) << "[ERROR] Failed to send data at " << __FUNCTION__ << ": " << strerror(errno) << " (" << errno << ") "
        //                   << "\n";
        //     }
        // }
    }

    bool XDRClient::sendBuffer(char *buffer, int len)
    {
        if (sendto(serverSocket, buffer, len, 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
        {
            ASA_LOG(error) << "[ERROR] Failed to send data at " << __FUNCTION__ << ": " << strerror(errno) << " (" << errno << ") "
                      << "\n";
            return false;
        }
        return true;
    }
    bool XDRClient::send(
        const char *model,
        const double currentPosition[3],
        const float currentOrientation[3],
        const float linearVel[3],
        const float angularVel[3],
        const float linearAccel[3],
        const float angularAccel[3],
        const char *callsign,
        std::vector<FGPropertyData *> *properties,
        char *extra,
        int extra_len)
    {
        // std::cout << "Sending data...\n";

        // Prepare and send the message
        T_PositionMsg positionMsg;
        populatePositionMsg(
            positionMsg,
            model,
            currentPosition,
            currentOrientation,
            linearVel,
            angularVel,
            linearAccel,
            angularAccel,
            1.0, // lag
            0.0  // pad
        );

        T_MsgHdr msgHdr;
        populateMsgHdr(
            msgHdr,
            MSG_MAGIC,
            PROTO_VER,
            POS_DATA_ID,
            sizeof(T_MsgHdr) + sizeof(T_PositionMsg) + extra_len,
            0,
            100,
            0,
            callsign);

        // Combine the header and message into a single buffer

        char buffer[MAX_PACKET_SIZE];
        memset(buffer, 0xff, MAX_PACKET_SIZE);
        std::memcpy(buffer, &msgHdr, sizeof(msgHdr));
        std::memcpy(buffer + sizeof(msgHdr), &positionMsg, sizeof(positionMsg));

        if (extra_len > 0)
        {
            std::memcpy(buffer + sizeof(msgHdr) + sizeof(positionMsg), extra, extra_len);
        }

        if (properties != nullptr && properties->size() > 0)
        {
            // TODO continuar aqui
            ASA_LOG(error) << "[WARNING] properties not sent. Decode not implemented yet." << std::endl;
        }
        int len = sizeof(T_MsgHdr) + sizeof(T_PositionMsg) + extra_len;
        if (extra_len > 0)
        {
            asa::fg::MsgBuf msg;
            asa::fg::FGExternalMotionData motionInfo;
            memcpy(msg.Msg, buffer, sizeof(msg.Msg));
            if (!ProcessPosMsg(msg, &motionInfo))
            {
                ASA_LOG(error) << "Error processing message" << std::endl;
            }
            std::cout << "#################################################" << std::endl;
        }

        return sendBuffer(buffer, len);
    }

    bool XDRClient::recvData(MsgBuf *msg)
    {

        socklen_t receiveAddrLen = sizeof(serverAddress);
        // struct pollfd pfd = {.fd = serverSocket, .events = POLLIN};
        // poll(&pfd, 1, 1000);

        ssize_t recv = recvfrom(serverSocket, msg->Msg, MAX_PACKET_SIZE, 0, (struct sockaddr *)&serverAddress, &receiveAddrLen);

        // if(!(pfd.revents & POLLIN)){
        //     ASA_LOG(error) << "[WARNING] receive timeout. " << "\n";
        //     return false;
        // }

        if (recv == -1)
        {
            //ASA_LOG(error) << "[ERROR] Failed to receive data: " << strerror(errno) << " (" << errno << ")\n";
            return false;
        }
        else if (recv == 0)
        {
            ASA_LOG(error) << "[ERROR] Connection closed by server!\n";
            return false;
        }

        // check protocol magic code and protocol version
        T_MsgHdr *msgHdr;
        msgHdr = msg->msgHdr();

        // Decode specific values from the received message
        uint32_t magic = XDR_decode_uint32(msgHdr->Magic);
        if (magic != MSG_MAGIC)
        {
            ASA_LOG(error) << "[ERROR] Invalid Magic code: " << magic << "." << std::endl;
            return false;
        }

        uint32_t version = XDR_decode_uint32(msgHdr->Version);
        if (version != PROTO_VER)
        {
            ASA_LOG(error) << "[ERROR] Invalid Protocol version: " << version << "." << std::endl;
            return false;
        }

        return true;
    }

    void XDRClient::receive()
    {

        int counter = 0;
        while (true)
        {

            // std::cout << "Receiving data...\n";
            MsgBuf msg;
            T_MsgHdr *msgHdr;

            if (recvData(&msg))
            {
                std::stringstream ss;
                FGExternalMotionData motionInfo;
                // memcpy(msg.Msg, buffer, sizeof(msg.Msg));
                ProcessPosMsg(msg, &motionInfo);

                msgHdr = msg.msgHdr();

                // Decode specific values from the received message
                uint32_t magic = XDR_decode_uint32(msgHdr->Magic);
                uint32_t version = XDR_decode_uint32(msgHdr->Version);
                //
                std::cout.precision(6);
                std::cout << std::fixed;
                std::cout << "################################################################################" << std::endl;
                std::cout << "Received data: " << msg.Header.Callsign << " : " << magic << " : " << version << ":" << counter++ << std::endl;
                // std::cout << '\t' << "(lag,time): (" << motionInfo.lag << "," << motionInfo.time << ")" << std::endl;
                std::cout << '\t' << "position: (" << motionInfo.position[0] << "," << motionInfo.position[1] << "," << motionInfo.position[2] << ")" << std::endl;
                std::cout << '\t' << "linearVel: (" << motionInfo.linearVel[0] << "," << motionInfo.linearVel[1] << "," << motionInfo.linearVel[2] << ")" << std::endl;
                std::cout << '\t' << "linearAccel: (" << motionInfo.linearAccel[0] << "," << motionInfo.linearAccel[1] << "," << motionInfo.linearAccel[2] << ")" << std::endl;
                std::cout << '\t' << "angularVel: (" << motionInfo.angularVel[0] << "," << motionInfo.angularVel[1] << "," << motionInfo.angularVel[2] << ")" << std::endl;
                std::cout << '\t' << "angularAccel: (" << motionInfo.angularAccel[0] << "," << motionInfo.angularAccel[1] << "," << motionInfo.angularAccel[2] << ")" << std::endl;
                // for (auto p : motionInfo.properties)
                // {
                //     sprintProperty(ss, p);
                // }
                std::cout << ss.str() << std::endl;
            }
        }
    }

    void XDRClient::populatePositionMsg(
        T_PositionMsg &positionMsg,
        const char *model,
        const double currentPosition[3],
        const float currentOrientation[3],
        const float linearVel[3],
        const float angularVel[3],
        const float linearAccel[3],
        const float angularAccel[3],
        double lag,
        double pad)
    {
        strncpy(positionMsg.Model, model, MAX_MODEL_NAME_LEN - 1);
        positionMsg.Model[MAX_MODEL_NAME_LEN - 1] = '\0';
        positionMsg.time = XDR_encode_double(static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count()));
        positionMsg.lag = XDR_encode_double(lag);

        for (unsigned i = 0; i < 3; ++i)
        {
            positionMsg.position[i] = XDR_encode_double(currentPosition[i]);
            positionMsg.orientation[i] = XDR_encode_float(currentOrientation[i]);
            positionMsg.linearVel[i] = XDR_encode_float(linearVel[i]);
            positionMsg.angularVel[i] = XDR_encode_float(angularVel[i]);
            positionMsg.linearAccel[i] = XDR_encode_float(linearAccel[i]);
            positionMsg.angularAccel[i] = XDR_encode_float(angularAccel[i]);
        }

        positionMsg.pad = XDR_encode_double(pad);
    }

    void XDRClient::populateMsgHdr(
        T_MsgHdr &msgHdr,
        uint32_t magic,
        uint32_t version,
        uint32_t msgId,
        uint32_t msgLen,
        uint32_t requestedRangeNmMin,
        uint32_t requestedRangeNmMax,
        uint16_t replyPort,
        const char *callsign)
    {
        // Populate msgHdr fields as needed
        // Writing the Header (some fixed or default values)
        msgHdr.Magic = XDR_encode_uint32(magic);
        msgHdr.Version = XDR_encode_uint32(version);
        msgHdr.MsgId = XDR_encode_uint32(msgId);
        msgHdr.MsgLen = XDR_encode_uint32(msgLen);
        msgHdr.RequestedRangeNm = XDR_encode_shortints32(requestedRangeNmMin, requestedRangeNmMax);
        msgHdr.ReplyPort = XDR_encode_uint16(replyPort);
        strncpy(msgHdr.Callsign, callsign, MAX_CALLSIGN_LEN);
        msgHdr.Callsign[MAX_CALLSIGN_LEN - 1] = '\0';
    }
}