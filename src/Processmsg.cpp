#include "asa/fg/Processmsg.hpp"
#include "asa/core/base/Log.hpp"

namespace asa::fg
{
    bool isSane(const FGExternalMotionData &motionInfo)
    {
        // check for corrupted data (NaNs)
        bool isCorrupted = false;
        isCorrupted |= ((SGMisc<double>::isNaN(motionInfo.time)) ||
                        (SGMisc<double>::isNaN(motionInfo.lag)) ||
                        (isnan(motionInfo.orientation(3))));
        for (unsigned i = 0; (i < 3) && (!isCorrupted); ++i)
        {
            isCorrupted |= ((isnan(motionInfo.position(i))) ||
                            (isnan(motionInfo.orientation(i))) ||
                            (isnan(motionInfo.linearVel(i))) ||
                            (isnan(motionInfo.angularVel(i))) ||
                            (isnan(motionInfo.linearAccel(i))) ||
                            (isnan(motionInfo.angularAccel(i))));
        }
        return !isCorrupted;
    }

    const IdPropertyList *findProperty(unsigned id)
    {
        std::pair<const IdPropertyList *, const IdPropertyList *> result = std::equal_range(sIdPropertyList, sIdPropertyList + numProperties, id,
                                                                                            ComparePropertyId());
        if (result.first == result.second)
        {
            return 0;
        }
        else
        {
            return result.first;
        }
    }

    bool verifyProperties(const xdr_data_t *data, const xdr_data_t *end)
    {
        using namespace simgear;
        const xdr_data_t *xdr = data;
        while (xdr < end)
        {
            unsigned id = XDR_decode_uint32(*xdr);
            const IdPropertyList *plist = findProperty(id);

            if (plist)
            {
                xdr++;
                // How we decode the remainder of the property depends on the type
                switch (plist->type)
                {
                case props::INT:
                case props::BOOL:
                case props::LONG:
                    xdr++;
                    break;
                case props::FLOAT:
                case props::DOUBLE:
                {
                    float val = XDR_decode_float(*xdr);
                    if (SGMisc<float>::isNaN(val))
                        return false;
                    xdr++;
                    break;
                }
                case props::STRING:
                case props::UNSPECIFIED:
                {
                    // String is complicated. It consists of
                    // The length of the string
                    // The string itself
                    // Padding to the nearest 4-bytes.
                    // XXX Yes, each byte is padded out to a word! Too late
                    // to change...
                    uint32_t length = XDR_decode_uint32(*xdr);
                    xdr++;
                    // Old versions truncated the string but left the length
                    // unadjusted.
                    if (length > MAX_TEXT_SIZE)
                        length = MAX_TEXT_SIZE;
                    xdr += length;
                    // Now handle the padding
                    while ((length % 4) != 0)
                    {
                        xdr++;
                        length++;
                        // cout << "0";
                    }
                }
                break;
                default:
                    // cerr << "Unknown Prop type " << id << " " << type << "\n";
                    xdr++;
                    break;
                }
            }
            else
            {
                // give up; this is a malformed property list.
                return false;
            }
        }
        return true;
    }

    bool ProcessPosMsg(const MsgBuf &Msg, FGExternalMotionData *motionInfo)
    {
        const T_MsgHdr *MsgHdr = Msg.msgHdr();
        if (MsgHdr->MsgLen < sizeof(T_MsgHdr) + sizeof(T_PositionMsg))
        {
            //   SG_LOG(SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::MP_ProcessData - "
            //      << "Position message received with insufficient data");
            return false;
        }

        if (motionInfo->properties.size() > 0)
        {
            std::cerr << "Motion info not empty" << std::endl;
            return false;
        }

        const T_PositionMsg *PosMsg = Msg.posMsg();

        // int fallback_model_index;
        motionInfo->time = XDR_decode_double(PosMsg->time);
        motionInfo->lag = XDR_decode_double(PosMsg->lag);
        for (unsigned i = 0; i < 3; ++i)
            motionInfo->position(i) = XDR_decode_double(PosMsg->position[i]);
        SGVec3f angleAxis;
        for (unsigned i = 0; i < 3; ++i)
            angleAxis(i) = XDR_decode_float(PosMsg->orientation[i]);
        motionInfo->orientation = SGQuatf::fromAngleAxis(angleAxis);
        for (unsigned i = 0; i < 3; ++i)
            motionInfo->linearVel(i) = XDR_decode_float(PosMsg->linearVel[i]);
        for (unsigned i = 0; i < 3; ++i)
            motionInfo->angularVel(i) = XDR_decode_float(PosMsg->angularVel[i]);
        for (unsigned i = 0; i < 3; ++i)
            motionInfo->linearAccel(i) = XDR_decode_float(PosMsg->linearAccel[i]);
        for (unsigned i = 0; i < 3; ++i)
            motionInfo->angularAccel(i) = XDR_decode_float(PosMsg->angularAccel[i]);

        // sanity check: do not allow injection of corrupted data (NaNs)
        if (!isSane(*motionInfo))
        {
            // drop this message, keep old position until receiving valid data
            //   SG_LOG(SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::ProcessPosMsg - "
            //      << "Position message with invalid data (NaN) received from "
            //      << MsgHdr->Callsign);
            return false;
        }

        // cout << "INPUT MESSAGE\n";

        // There was a bug in 1.9.0 and before: T_PositionMsg was 196 bytes
        // on 32 bit architectures and 200 bytes on 64 bit, and this
        // structure is put directly on the wire. By looking at the padding,
        // we can sort through the mess, mostly:
        // If padding is 0 (which is not a valid property type), then the
        // message was produced by a new client or an old 64 bit client that
        // happened to have 0 on the stack;
        // Else if the property list starting with the padding word is
        // well-formed, then the client is probably an old 32 bit client and
        // we'll go with that;
        // Else it is an old 64-bit client and properties start after the
        // padding.
        // There is a chance that we could be fooled by garbage in the
        // padding looking like a valid property, so verifyProperties() is
        // strict about the validity of the property values.
        const xdr_data_t *xdr = Msg.properties();

        /*
         * with V2 we use the pad to forcefully invoke older clients to verify (and discard)
         * our new protocol.
         * This will preserve the position info but not transmit the properties; which is about
         * the most reasonable compromise we can have
         */
        int pad = XDR_decode_int32(PosMsg->pad);
        if (PosMsg->pad != 0 && pad != V2_PAD_MAGIC)
        {
            if (verifyProperties(&PosMsg->pad, Msg.propsRecvdEnd()))
                xdr = &PosMsg->pad;
            else if (!verifyProperties(xdr, Msg.propsRecvdEnd()))
                goto noprops;
        }
        while (xdr < Msg.propsRecvdEnd())
        {
            // First element is always the ID
            unsigned id = XDR_decode_uint32(*xdr);

            /*
             * As we can detect a short int encoded value (by the upper word being non-zero) we can
             * do the decode here; set the id correctly, extract the integer and set the flag.
             * This can then be picked up by the normal processing based on the flag
             */
            int int_value = 0;
            bool short_int_encoded = false;
            if (id & 0xffff0000)
            {
                int v1, v2;
                XDR_decode_shortints32(*xdr, v1, v2);
                int_value = v2;
                id = v1;
                short_int_encoded = true;
            }

            // if (pMultiPlayDebugLevel->getIntValue() & 8)
            //  if (true & 8)
            //  SG_LOG(SG_NETWORK, SG_INFO,
            //      "[RECV] add " << std::hex << xdr
            //      << std::dec <<
            //      ": buf[" << ((char*)xdr) - ((char*)data)
            //      << "] id=" << id
            //      << " SIenc " << short_int_encoded);

            // Check the ID actually exists and get the type
            const IdPropertyList *plist = findProperty(id);

            if (plist)
            {
                FGPropertyData *pData = new FGPropertyData;
                if (plist->decode_received)
                {
                    //
                    // this needs the pointer prior to the extraction of the property id and possible shortint decode
                    // too allow the method to redecode as it wishes
                    xdr = (*plist->decode_received)(plist, xdr, pData);
                }
                else
                {
                    pData->id = id;
                    pData->type = plist->type;
                    xdr++;
                    // How we decode the remainder of the property depends on the type
                    switch (pData->type)
                    {
                    case simgear::props::BOOL:
                        /*
                         * For 2017.2 we support boolean arrays transmitted as a single int for 30 bools.
                         * this section handles the unpacking into the arrays.
                         */
                        if (pData->id >= BOOLARRAY_START_ID && pData->id <= BOOLARRAY_END_ID)
                        {
                            unsigned int val = XDR_decode_uint32(*xdr);
                            bool first_bool = true;
                            xdr++;
                            for (int bitidx = 0; bitidx <= 30; bitidx++)
                            {
                                // ensure that this property is in the master list.
                                const IdPropertyList *plistBool = findProperty(id + bitidx);

                                if (plistBool)
                                {
                                    if (first_bool)
                                        first_bool = false;
                                    else
                                        pData = new FGPropertyData;

                                    pData->id = id + bitidx;
                                    pData->int_value = (val & (1 << bitidx)) != 0;
                                    pData->type = simgear::props::BOOL;
                                    motionInfo->properties.push_back(pData);

                                    // ensure that this is null because this section of code manages the property data and list directly
                                    // it has to be this way because one MP value results in multiple properties being set.
                                    pData = nullptr;
                                }
                            }
                            break;
                        }
                    case simgear::props::INT:
                    case simgear::props::LONG:
                        if (short_int_encoded)
                        {
                            pData->int_value = int_value;
                            pData->type = simgear::props::INT;
                        }
                        else
                        {
                            pData->int_value = XDR_decode_uint32(*xdr);
                            xdr++;
                        }
                        // cout << pData->int_value << "\n";
                        break;
                    case simgear::props::FLOAT:
                    case simgear::props::DOUBLE:
                        if (short_int_encoded)
                        {
                            switch (plist->TransmitAs)
                            {
                            case TT_SHORT_FLOAT_1:
                                pData->float_value = (double)int_value / 10.0;
                                break;
                            case TT_SHORT_FLOAT_2:
                                pData->float_value = (double)int_value / 100.0;
                                break;
                            case TT_SHORT_FLOAT_3:
                                pData->float_value = (double)int_value / 1000.0;
                                break;
                            case TT_SHORT_FLOAT_4:
                                pData->float_value = (double)int_value / 10000.0;
                                break;
                            case TT_SHORT_FLOAT_NORM:
                                pData->float_value = (double)int_value / 32767.0;
                                break;
                            default:
                                break;
                            }
                        }
                        else
                        {
                            pData->float_value = XDR_decode_float(*xdr);
                            xdr++;
                        }
                        break;
                    case simgear::props::STRING:
                    case simgear::props::UNSPECIFIED:
                    {
                        // if the string is using short int encoding then it is in the new format.
                        if (short_int_encoded)
                        {
                            uint32_t length = int_value;
                            pData->string_value = new char[length + 1];

                            char *cptr = (char *)xdr;
                            for (unsigned i = 0; i < length; i++)
                            {
                                pData->string_value[i] = *cptr++;
                            }
                            pData->string_value[length] = '\0';
                            xdr = (xdr_data_t *)cptr;
                        }
                        else
                        {
                            // String is complicated. It consists of
                            // The length of the string
                            // The string itself
                            // Padding to the nearest 4-bytes.
                            uint32_t length = XDR_decode_uint32(*xdr);
                            xdr++;
                            // cout << length << " ";
                            //  Old versions truncated the string but left the length unadjusted.
                            if (length > MAX_TEXT_SIZE)
                                length = MAX_TEXT_SIZE;
                            pData->string_value = new char[length + 1];
                            // cout << " String: ";
                            for (unsigned i = 0; i < length; i++)
                            {
                                pData->string_value[i] = (char)XDR_decode_int8(*xdr);
                                xdr++;
                            }

                            pData->string_value[length] = '\0';

                            // Now handle the padding
                            while ((length % 4) != 0)
                            {
                                xdr++;
                                length++;
                                // cout << "0";
                            }
                            // cout << "\n";
                        }
                    }
                    break;

                    default:
                        pData->float_value = XDR_decode_float(*xdr);
                        // SG_LOG(SG_NETWORK, SG_DEBUG, "Unknown Prop type " << pData->id << " " << pData->type);
                        xdr++;
                        break;
                    }
                }
                if (pData)
                {
                    motionInfo->properties.push_back(pData);

                    // Special case - we need the /sim/model/fallback-model-index to create
                    // the MP model
                    // if (pData->id == FALLBACK_MODEL_ID)
                    // {
                    // fallback_model_index = pData->int_value;
                    // ASA_LOG(debug) <<  "Found Fallback model index in message " << fallback_model_index;
                    //}
                }
            }
            else
            {
                // We failed to find the property. We'll try the next packet immediately.
                //   SG_LOG(SG_NETWORK, SG_DEBUG, "FGMultiplayMgr::ProcessPosMsg - "
                //          "message from " << MsgHdr->Callsign << " has unknown property id "
                //          << id);

                // At this point the packet must be considered to be unreadable
                // as we have no way of knowing the length of this property (it could be a string)
                break;
            }
        }
    noprops:

        return true;
    } // FGMultiplayMgr::ProcessPosMsg()

    short get_scaled_short(double v, double scale)
    {
        float nv = v * scale;
        if (nv >= 32767)
            return 32767;
        if (nv <= -32767)
            return -32767;
        short rv = (short)nv;
        return rv;
    }

    void FillMsgHdr(T_MsgHdr *MsgHdr, int MsgId, unsigned _len, std::string mCallsign)
    {
        uint32_t len;
        switch (MsgId)
        {
        case CHAT_MSG_ID:
            len = sizeof(T_MsgHdr) + sizeof(T_ChatMsg);
            break;
        case POS_DATA_ID:
            len = _len;
            break;
        default:
            len = sizeof(T_MsgHdr);
            break;
        }
        MsgHdr->Magic = XDR_encode_uint32(MSG_MAGIC);
        MsgHdr->Version = XDR_encode_uint32(PROTO_VER);
        MsgHdr->MsgId = XDR_encode_uint32(MsgId);
        MsgHdr->MsgLen = XDR_encode_uint32(len);
        MsgHdr->RequestedRangeNm = XDR_encode_shortints32(0, 100); // XDR_encode_shortints32(0,pMultiPlayRange->getIntValue());
        MsgHdr->ReplyPort = REPLY_PORT_DFT;
        strncpy(MsgHdr->Callsign, mCallsign.c_str(), MAX_CALLSIGN_LEN);
        MsgHdr->Callsign[MAX_CALLSIGN_LEN - 1] = '\0';
    }

    void SendMyPosition(const FGExternalMotionData &motionInfo,
                        int protocolToUse,
                        std::string model,
                        double timeAccel,
                        std::string callSign,
                        int serverSocket,
                        struct sockaddr *serverAddress)
    {

        if (!isSane(motionInfo))
        {
            // Current local data is invalid (NaN), so stop MP transmission.
            // => Be nice to older FG versions (no NaN checks) and don't waste bandwidth.
            // SG_LOG(SG_NETWORK, SG_ALERT, "FGMultiplayMgr::SendMyPosition - "
            //<< "Local data is invalid (NaN). Data not transmitted.");
            return;
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
            PosMsg->position[i] = XDR_encode_double(motionInfo.position(i));
        SGVec3f angleAxis;
        motionInfo.orientation.getAngleAxis(angleAxis);
        for (unsigned i = 0; i < 3; ++i)
            PosMsg->orientation[i] = XDR_encode_float(angleAxis(i));

        // including speed up time in velocity and acceleration

        for (unsigned i = 0; i < 3; ++i)
            PosMsg->linearVel[i] = XDR_encode_float(motionInfo.linearVel(i) * timeAccel);
        for (unsigned i = 0; i < 3; ++i)
            PosMsg->angularVel[i] = XDR_encode_float(motionInfo.angularVel(i) * timeAccel);
        for (unsigned i = 0; i < 3; ++i)
            PosMsg->linearAccel[i] = XDR_encode_float(motionInfo.linearAccel(i) * timeAccel * timeAccel);
        for (unsigned i = 0; i < 3; ++i)
            PosMsg->angularAccel[i] = XDR_encode_float(motionInfo.angularAccel(i) * timeAccel * timeAccel);

        xdr_data_t *ptr = msgBuf.properties();
        xdr_data_t *data = ptr;

        xdr_data_t *msgEnd = msgBuf.propsEnd();

        // if (pMultiPlayDebugLevel->getIntValue())
        //     msgBuf.zero();
        struct BoolArrayBuffer boolBuffer[MAX_BOOL_BUFFERS];
        memset(&boolBuffer, 0, sizeof(boolBuffer));

        PropertyDefinitionMap mPropertyDefinition;
        for (int partition = 1; partition <= protocolToUse; partition++)
        {
            std::vector<FGPropertyData *>::const_iterator it = motionInfo.properties.begin();
            while (it != motionInfo.properties.end())
            {
                const struct IdPropertyList *propDef = &(sIdPropertyList[(*it)->id]);

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

                    ASA_LOG(warning) << "[SEND] pt " << partition << ": buf[" << (ptr - data) * sizeof(*ptr)
                                     << "] id=" << (*it)->id << " type " << transmit_type;

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

        ASA_LOG(warning) << "[SEND] Packet len " << msgLen;

        ASA_LOG(warning) << data << (ptr - data) * sizeof(*ptr);
        /*
         * simple loopback of ourselves - to enable easy MP debug for model developers; see (1) above
         */

        // bool ProcessMsgBuf(const MsgBuf& Msg, FGExternalMotionData* motionInfo)
        FGExternalMotionData mi;
        ProcessPosMsg(msgBuf, &mi);

        if (msgLen > 0)
        {

            if (sendto(serverSocket, msgBuf.Msg, msgLen, 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
            {
                std::cerr << "[ERROR] Failed to send data at " << __FUNCTION__ << ": " << strerror(errno) << " (" << errno << ") "
                          << "\n";
            }
        }
    }

}