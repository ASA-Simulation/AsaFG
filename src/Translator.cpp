#include "asa/fg/Translator.hpp"
#include "asa/core/base/Log.hpp"

namespace asa::fg
{

    Translator::Translator()
    {
    }
    Translator::~Translator()
    {
    }

    bool Translator::unserialize(MsgBuf &Msg, FGExternalMotionData *motionInfo)
    {

        const T_MsgHdr *MsgHdr = Msg.msgHdr();
        if (MsgHdr->MsgLen < sizeof(T_MsgHdr) + sizeof(T_PositionMsg))
        {
            ASA_LOG(warning) << "FGMultiplayMgr::MP_ProcessData - "
                             << "Position message received with insufficient data";
            return false;
        }

        if (motionInfo->properties.size() > 0)
        {
            ASA_LOG(warning) << "Motion info not empty";
            return false;
        }

        const T_PositionMsg *PosMsg = Msg.posMsg();

        int fallback_model_index = 0;
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
            ASA_LOG(warning) << "FGMultiplayMgr::ProcessPosMsg - "
                             << "Position message with invalid data (NaN) received from "
                             << MsgHdr->Callsign;
            return false;
        }

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

            std::cout << id << std::endl;

            if (id == 811)
            {
                std::cout << std::endl;
            }

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
                            //  Old versions truncated the string but left the length unadjusted.
                            if (length > MAX_TEXT_SIZE)
                                length = MAX_TEXT_SIZE;
                            pData->string_value = new char[length + 1];
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
                            }
                        }
                    }
                    break;

                    default:
                        pData->float_value = XDR_decode_float(*xdr);
                        ASA_LOG(warning) << "Unknown Prop type " << pData->id << " " << pData->type;
                        xdr++;
                        break;
                    }
                }
                if (pData)
                {
                    motionInfo->properties.push_back(pData);

                    // Special case - we need the /sim/model/fallback-model-index to create
                    // the MP model
                    if (pData->id == FALLBACK_MODEL_ID)
                    {
                        fallback_model_index = pData->int_value;
                        ASA_LOG(debug) << "Found Fallback model index in message " << fallback_model_index;
                    }
                }
            }
            else
            {
                // We failed to find the property. We'll try the next packet immediately.
                ASA_LOG(warning) << "FGMultiplayMgr::ProcessPosMsg - "
                                    "message from "
                                 << MsgHdr->Callsign << " has unknown property id "
                                 << id;

                // At this point the packet must be considered to be unreadable
                // as we have no way of knowing the length of this property (it could be a string)
                break;
            }
        }
    noprops:

        return true;
    }

    int Translator::serialize(MsgBuf *msgBuf, FGExternalMotionData *motionData, std::string model, std::string callSign)
    {
        return 0;
    }

}