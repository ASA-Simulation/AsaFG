#ifndef __asa_fg_static_notification_H__
#define __asa_fg_static_notification_H__

#include "FGNotification.hpp"
#include "Emesary.hpp"
#include <string>

namespace asa::fg
{

    class StaticNotification : public FGNotification
    {
    public:
        StaticNotification(int typeId);
        StaticNotification(int typeId, geoCoord position, int kind, int secondaryKind, double heading, int flags1, int flags2, int uniqueIdentity);
        StaticNotification(int typeId, std::string buf);

        
        virtual size_t bufSize() const;
        virtual std::string encode();
        bool decode(std::string);

        int getKind() const { return kind; }
        void setKind(int kind_) { kind = kind_; }

        int getSecondaryKind() const { return secondaryKind; }
        void setSecondaryKind(int secondaryKind_) { secondaryKind = secondaryKind_; }

        int getIsDistinct() const { return isDistinct; }
        void setIsDistinct(int isDistinct_) { isDistinct = isDistinct_; }

        std::string getCallsign() const { return Callsign; }
        void setCallsign(const std::string &Callsign_) { Callsign = Callsign_; }

        geoCoord getPosition() const { return position; }
        void setPosition(geoCoord position_) { position = position_; }

        double getHeading() const { return heading; }
        void setHeading(double heading_) { heading = heading_; }

        int getFlags1() const { return flags1; }
        void setFlags1(int flags1_) { flags1 = flags1_; }

        int getFlags2() const { return flags2; }
        void setFlags2(int flags2_) { flags2 = flags2_; }

    
    private:
        
        geoCoord position;
        int kind;                       // 1=create, 2=move, 3=delete, 4=request_all
        int secondaryKind;              // 0 = small crater, 1 = big crater, 2 = smoke
        double heading;
        int flags1;                     //7 bits for whatever.  
        int flags2;                     //7 bits for whatever.   
        int uniqueIdentity; 
        int isDistinct;                 //keep it 0
        std::string Callsign;           //populated automatically by the incoming bridge when routed
        
    };

}

#endif /* __asa_fg_static_notification_H__ */