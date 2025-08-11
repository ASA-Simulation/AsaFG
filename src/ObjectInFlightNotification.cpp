#include "asa/fg/ObjectInFlightNotification.hpp"
#include "asa/fg/Emesary.hpp"

namespace asa::fg
{

    ObjectInFlightNotification::ObjectInFlightNotification(int typeId) : FGNotification(typeId), kind(0), secondaryKind(0), uniqueIdentity(0), uniqueIndex(0), isDistinct(1), Callsign("")
    {
        // EMPTY
    }

    ObjectInFlightNotification::ObjectInFlightNotification(int typeId, geoCoord position, int kind, int secondaryKind, int uniqueIdentity) : FGNotification(typeId), position(position), kind(kind), secondaryKind(secondaryKind), uniqueIdentity(uniqueIdentity)
    {
        // EMPTY
    }

    ObjectInFlightNotification::ObjectInFlightNotification(int typeId, std::string buf) : ObjectInFlightNotification::ObjectInFlightNotification(typeId)
    {
        this->decode(buf);
    }

    size_t ObjectInFlightNotification::bufSize() const
    {
        // position +  kind + secondaryKind + uniqueIdentity
        // (4+4+3)  +   1   +       1       +    1
        return 14;
    }

    std::string ObjectInFlightNotification::encode()
    {
        std::string buf;

        buf += TransferCoord::encode(this->position);
        buf += TransferByte::encode(this->kind);
        buf += TransferByte::encode(this->secondaryKind);
        buf += TransferByte::encode(this->uniqueIdentity);

        return buf;
    }

    bool ObjectInFlightNotification::decode(std::string buf)
    {
        ReturnValueDouble rvd;
        ReturnValueCoord rvc;
        int pos = 0;

        // 18 + 1 (remoteCallsign length)
        if (buf.length() < 14)
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
