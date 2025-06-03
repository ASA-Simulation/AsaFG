#ifndef __asa_fg_object_in_flight_notification_H__
#define __asa_fg_object_in_flight_notification_H__

#include "FGNotification.hpp"
#include "Emesary.hpp"
#include <string>

namespace asa::fg
{
    
    class ObjectInFlightNotification : public FGNotification
    {
    public:
        ObjectInFlightNotification(int typeId);
        ObjectInFlightNotification(int typeId, geoCoord position, int kind, int secondaryKind, int uniqueIdentity);
        ObjectInFlightNotification(int typeId, std::string buf);

        
        virtual size_t bufSize() const;
        virtual std::string encode();
        bool decode(std::string);

        int getKind() const { return kind; }
        void setKind(int kind_) { kind = kind_; }

        int getSecondaryKind() const { return secondaryKind; }
        void setSecondaryKind(int secondaryKind_) { secondaryKind = secondaryKind_; }

        geoCoord getPosition() const { return position; }
        void setPosition(geoCoord position_) { position = position_; }

        int getUniqueIndex() const { return uniqueIndex; }
        void setUniqueIndex(int uniqueIndex_) { uniqueIndex = uniqueIndex_; }

        int getIsDistinct() const { return isDistinct; }
        void setIsDistinct(int isDistinct_) { isDistinct = isDistinct_; }

        std::string getCallsign() const { return Callsign; }
        void setCallsign(const std::string &Callsign_) { Callsign = Callsign_; }

        int getUniqueIdentity() const { return uniqueIdentity; }
        void UniqueIdentity(int uniqueIdentity_) { uniqueIdentity = uniqueIdentity_; }

    private:
        
        geoCoord position;
        int kind;         
        int secondaryKind;
        int uniqueIdentity;         
        int uniqueIndex;
        int isDistinct;                 
        std::string Callsign;           //populated automatically by the incoming bridge when routed   
        
    };

}
#endif /* __asa_fg_object_in_flight_notification_H__ */