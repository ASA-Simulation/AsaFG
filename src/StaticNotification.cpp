#include "asa/fg/StaticNotification.hpp"
#include "asa/fg/Emesary.hpp"

namespace asa::fg
{

    StaticNotification::StaticNotification(int typeId) : FGNotification(typeId), kind(0), secondaryKind(0), heading(360), flags1(0), flags2(0), uniqueIdentity(0), isDistinct(0), Callsign("")
    {
        // EMPTY
    }

    StaticNotification::StaticNotification(int typeId, geoCoord position, int kind, int secondaryKind, double heading, int flags1, int flags2, int uniqueIdentity) : FGNotification(typeId), position(position), kind(kind), secondaryKind(secondaryKind), heading(heading), flags1(flags1), flags2(flags2), uniqueIdentity(uniqueIdentity)
    {
        // EMPTY
    }

    StaticNotification::StaticNotification(int typeId, std::string buf) : StaticNotification::StaticNotification(typeId)
    {
        this->decode(buf);
    }

    size_t StaticNotification::bufSize() const
    {
        // position +  kind + secondaryKind + heading + flags1 + flags2 + uniqueIdentity
        // (4+4+3)  +   1   +       1       +    1     +   1    +   1   +   3
        return 19;
    }

    std::string StaticNotification::encode()
    {
        std::string buf;

        buf += TransferCoord::encode(this->position);
        buf += TransferByte::encode(this->kind);
        buf += TransferByte::encode(this->secondaryKind);
        buf += TransferFixedDouble::encode(normdeg180(this->heading), 1, 1.54);
        buf += TransferByte::encode(this->flags1);
        buf += TransferByte::encode(this->flags2);
        buf += TransferInt::encode(this->uniqueIdentity, 3);

        return buf;
    }

    bool StaticNotification::decode(std::string buf)
    {
        ReturnValueDouble rvd;
        ReturnValueCoord rvc;
        int pos = 0;

        // 18 + 1 (remoteCallsign length)
        if (buf.length() < 19)
        {
            // ASA_LOG(error) << "Invalid buff length.";
            return false;
        }

        rvc = TransferCoord::decode(buf, pos);
        if (rvc.pos != 11)
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

        rvd = TransferFixedDouble::decode(buf, 1, 1.54, pos++);
        if (rvd.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'heading'.";
            return false;
        }
        this->heading = normdeg(rvd.value);

        rvd = TransferByte::decode(buf, pos++);
        if (rvd.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'flags1'.";
            return false;
        }
        this->flags1 = rvd.value;

        rvd = TransferByte::decode(buf, pos++);
        if (rvd.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'flags2'.";
            return false;
        }
        this->flags2 = rvd.value;

        rvd = TransferInt::decode(buf, 3, pos++);
        if (rvd.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'uniqueIdentity'.";
            return false;
        }
        this->uniqueIdentity = rvd.value;

        return true;
    }
}
