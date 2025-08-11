#include "asa/fg/ArmamentInFlightNotification.hpp"
#include "asa/fg/Emesary.hpp"

namespace asa::fg
{
    ArmamentInFlightNotification::ArmamentInFlightNotification(int typeId, double time) : FGNotification(typeId),
                                                                                          kind(0),
                                                                                          secondaryKind(0),
                                                                                          uniqueIndex(0),
                                                                                          u_fps(0),
                                                                                          heading(360),
                                                                                          pitch(90),
                                                                                          isDistinct(0),
                                                                                          Callsign(""),
                                                                                          remoteCallsign(""),
                                                                                          flags(0),
                                                                                          uniqueIdentity(0),
                                                                                          time(time)

    {
        // EMPTY
    }

    ArmamentInFlightNotification::ArmamentInFlightNotification(int typeId,
                                                               geoCoord position,
                                                               int kind,
                                                               int secondaryKind,
                                                               double u_fps,
                                                               double heading,
                                                               double pitch,
                                                               std::string remoteCallsign,
                                                               int flags,
                                                               int uniqueIdentity,
                                                               double time) : FGNotification(typeId),
                                                                              position(position),
                                                                              kind(kind),
                                                                              secondaryKind(secondaryKind),
                                                                              u_fps(u_fps),
                                                                              heading(heading),
                                                                              pitch(pitch),
                                                                              remoteCallsign(remoteCallsign),
                                                                              flags(flags),
                                                                              uniqueIdentity(uniqueIdentity),
                                                                              time(time)

    {
        // EMPTY
    }

    ArmamentInFlightNotification::ArmamentInFlightNotification(int typeId, std::string buf, double time) : ArmamentInFlightNotification::ArmamentInFlightNotification(typeId, time)
    {
        this->decode(buf);
    }

    size_t ArmamentInFlightNotification::bufSize() const
    {
        // kind + secondaryKind + Position +   u_fps  +  Heading + Pitch  + flags + uniqueIdentity +  RemoteCallsign
        //  1   +       1       + (4+4+3)  +     1    +    1     +   1    +   1   +   1            +  remoteCallsign (maxlen = 16)
        return 18 + this->remoteCallsign.length();
    }

    std::string ArmamentInFlightNotification::encode()
    {
        std::string buf;

        buf += TransferCoord::encode(this->position);
        buf += TransferByte::encode(this->kind);
        buf += TransferByte::encode(this->secondaryKind);
        buf += TransferFixedDouble::encode(this->u_fps - 3348, 1, 1 / 0.03703);
        buf += TransferFixedDouble::encode(normdeg180(this->heading), 1, 1.54);
        buf += TransferFixedDouble::encode(this->pitch, 1, 1 / 1.38);
        buf += TransferString::encode(this->remoteCallsign);
        buf += TransferByte::encode(this->flags);
        buf += TransferByte::encode(this->uniqueIdentity);

        return buf;
    }

    bool ArmamentInFlightNotification::decode(std::string buf)
    {

        ReturnValueDouble rvd;
        ReturnValueCoord rvc;
        ReturnValueString rvs;
        ReturnValueInt rdi;
        int pos = 0;

        // se a string recebida contém seperatorChar '!' ou endChar
        if (buf.find('!') != std::string::npos)
        {
            pos += 1; // skip !

            rdi = BinaryAsciiTransfer::decodeInt(buf, 4, pos);
            pos += 4;
            if (rdi.pos != pos)
            {
                // ASA_LOG(error) << "Error decoding 'relativeAltitude'.";
                return false;
            }
            this->msgID = rdi.value;

            pos += 1; // skip !
            rdi = BinaryAsciiTransfer::decodeInt(buf, 1, pos);
            pos += 1;
            if (rdi.pos != pos)
            {
                // ASA_LOG(error) << "Error decoding 'relativeAltitude'.";
                return false;
            }
            this->msgType = rdi.value;
            pos += 1; // skip !
        }

        // 18 + 1 (remoteCallsign length)
        if (buf.length() < 27)
        {
            // ASA_LOG(error) << "Invalid buff length.";
            return false;
        }

        rvc = TransferCoord::decode(buf, pos);
        if (rvc.pos != 19)
        {
            // ASA_LOG(error) << "Error decoding 'position'.";
            return false;
        }
        this->position = rvc.value;
        pos = rvc.pos;

        rvd = TransferByte::decode(buf, pos++);
        if (rvd.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'kind'.";
            return false;
        }
        this->kind = int(rvd.value);

        rvd = TransferByte::decode(buf, pos++);
        if (rvd.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'secondaryKind'.";
            return false;
        }
        this->secondaryKind = int(rvd.value);

        rvd = TransferFixedDouble::decode(buf, 1, 1 / 0.03703, pos++);
        if (rvd.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'u_fps'.";
            return false;
        }
        this->u_fps = rvd.value + 3348;

        rvd = TransferFixedDouble::decode(buf, 1, 1.54, pos++);
        if (rvd.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'heading'.";
            return false;
        }
        this->heading = normdeg(rvd.value);

        rvd = TransferFixedDouble::decode(buf, 1, 1 / 1.38, pos++);
        if (rvd.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'pitch'.";
            return false;
        }
        this->pitch = rvd.value;

        rvs = TransferString::decode(buf, pos);
        this->remoteCallsign = rvs.value;
        if (remoteCallsign.length() == 0)
        {
            // ASA_LOG(warning) << "Empty RemoteCallsing.";
        }
        if (rvs.pos == 0)
        {
            pos += 1;
        }
        else
        {
            pos = rvs.pos;
        }

        rvd = TransferByte::decode(buf, pos++);
        if (rvd.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'flags'.";
            return false;
        }
        this->flags = rvd.value;

        rvd = TransferByte::decode(buf, pos++);
        if (rvd.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'uniqueIdentity'.";
            return false;
        }
        this->uniqueIdentity = rvd.value;

        return true;
    }
}
