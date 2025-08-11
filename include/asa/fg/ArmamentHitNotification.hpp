#ifndef __asa_fg_armament_hit_notification_H__
#define __asa_fg_armament_hit_notification_H__

#include "asa/fg/FGNotification.hpp"
#include <string>

namespace asa::fg
{


    class ArmamentHitNotification : public FGNotification
    {
    public:
        ArmamentHitNotification(int typeId);
        ArmamentHitNotification(int typeId, int kind, int secondaryKind, double relativeAltitude, double distance, double bearing,
                             std::string remoteCallsign, int uniqueIdentity);
        ArmamentHitNotification(int typeId, std::string buf);
        virtual size_t bufSize() const;
        virtual std::string encode();
        bool decode(std::string);

        int getKind() const { return kind; }
        void setKind(int kind_) { kind = kind_; }

        int getSecondaryKind() const { return secondaryKind; }
        void setSecondaryKind(int secondaryKind_) { secondaryKind = secondaryKind_; }

        double getRelativeAltitude() const { return relativeAltitude; }
        void setRelativeAltitude(double relativeAltitude_) { relativeAltitude = relativeAltitude_; }

        double getDistance() const { return distance; }
        void setDistance(double distance_) { distance = distance_; }

        double getBearing() const { return bearing; }
        void setBearing(double bearing_) { bearing = bearing_; }

        std::string getRemoteCallsign() const { return remoteCallsign; }
        void setRemoteCallsign(const std::string &remoteCallsign_) { remoteCallsign = remoteCallsign_; }

        int getUniqueIdentity() const { return uniqueIdentity; }
        void UniqueIdentity(int uniqueIdentity_) { uniqueIdentity = uniqueIdentity_; }

        int getMsgID() const {return msgID;}
        void setMsgID(double msgID_) { msgID = msgID_; }

        int getMsgType() const {return msgType;}
        void setMsgType(double msgType_) { msgType = msgType_; }


    private:
        int msgID;
        int msgType;
        int kind;
        int secondaryKind;
        double relativeAltitude;
        double distance;
        double bearing;
        std::string remoteCallsign;
        int uniqueIdentity;
    };

}
#endif /* __asa_fg_armament_hit_notification_H__ */

       
