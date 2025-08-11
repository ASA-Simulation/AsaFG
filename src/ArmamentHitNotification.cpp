#include "asa/fg/ArmamentHitNotification.hpp"
#include "asa/fg/Emesary.hpp"

// asa::core
// #include "asa/core/base/Log.hpp"

namespace asa::fg
{
    ArmamentHitNotification::ArmamentHitNotification(int typeId) : FGNotification(typeId), kind(0), secondaryKind(0), 
                                               relativeAltitude(0), distance(0), bearing(0), remoteCallsign(""),
                                                                                          uniqueIdentity(0)
    {
        // EMPTY
    }
    ArmamentHitNotification::ArmamentHitNotification(int typeId, int kind, int secondaryKind, double relativeAltitude, double distance, double bearing,
                                               std::string remoteCallsign, int  uniqueIdentity): FGNotification(typeId), kind(kind), secondaryKind(secondaryKind), 
                                               relativeAltitude(relativeAltitude), distance(distance), bearing(bearing), remoteCallsign(remoteCallsign), uniqueIdentity(uniqueIdentity)
    {
        // EMPTY
    }

    ArmamentHitNotification::ArmamentHitNotification(int typeId, std::string buf) : ArmamentHitNotification::ArmamentHitNotification(typeId)
    {
        this->decode(buf);
    }

    size_t ArmamentHitNotification::bufSize() const
    {
        // kind + secondaryKind + relativeAltitude + distance + bearing + #remoteCallsign + remoteCallsign
        //  1   +     1         +        2         +     2    +    1    +        1        + remoteCallsign
        return 8 + this->remoteCallsign.length();
    }

    std::string ArmamentHitNotification::encode()
    {
        std::string buf;

        buf += TransferByte::encode(this->kind);
        buf += TransferByte::encode(this->secondaryKind);
        buf += TransferFixedDouble::encode(this->relativeAltitude, 2, 1. / 10.);
        buf += TransferFixedDouble::encode(this->distance, 2, 1. / 10.);
        buf += TransferFixedDouble::encode(normdeg180(this->bearing), 1, 1.54);
        buf += TransferString::encode(this->remoteCallsign);
        buf += TransferByte::encode(this->uniqueIdentity);

        return buf;
    }

    bool ArmamentHitNotification::decode(std::string buf)
    {
        ReturnValueInt rdi;
        ReturnValueDouble rdv;
        ReturnValueString rvs;
        int pos = 0;

        
       
        // se a string recebida contém seperatorChar '!' ou endChar
        if (buf.find('!') != std::string::npos) {
            pos+=1; //skip !

            rdi = BinaryAsciiTransfer::decodeInt(buf,4, pos);
            pos+=4;
            if (rdi.pos != pos)
            {
                // ASA_LOG(error) << "Error decoding 'relativeAltitude'.";
                return false;
            }
            this->msgID = rdi.value;

            pos+=1; //skip !
            rdi = BinaryAsciiTransfer::decodeInt(buf,1, pos);
            pos+=1;
            if (rdi.pos != pos)
            {
                // ASA_LOG(error) << "Error decoding 'relativeAltitude'.";
                return false;
            }
            this->msgType = rdi.value;
            pos+=1; //skip !



        }

        

        // 8 + 1 (remoteCallsign length)
        if (buf.length() < 9)
        {
            // ASA_LOG(error) << "Invalid buff length.";
            return false;
        }

        rdv = TransferByte::decode(buf, pos++);
        if (rdv.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'kind'.";
            return false;
        }
        this->kind = int(rdv.value);

        rdv = TransferByte::decode(buf, pos++);
        if (rdv.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'secondaryKind'.";
            return false;
        }
        this->secondaryKind = int(rdv.value);

        rdv = TransferFixedDouble::decode(buf, 2, 1. / 10., pos);
        pos += 2;
        if (rdv.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'relativeAltitude'.";
            return false;
        }
        this->relativeAltitude = rdv.value;

        rdv = TransferFixedDouble::decode(buf, 2, 1. / 10., pos);
        pos += 2;
        if (rdv.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'distance'.";
            return false;
        }
        this->distance = rdv.value;

        rdv = TransferFixedDouble::decode(buf, 1, 1.54, pos++);
        if (rdv.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'bearing'.";
            return false;
        }
        this->bearing = normdeg(rdv.value);

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


        rdv = TransferByte::decode(buf, pos++);
        if (rdv.pos != pos)
        {
            // ASA_LOG(error) << "Error decoding 'uniqueIdentity'.";
            return false;
        }
        this->uniqueIdentity = rdv.value;

        return true;
    }
}