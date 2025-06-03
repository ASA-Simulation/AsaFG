#ifndef __asa_fg_armament_in_flight_notification_H__
#define __asa_fg_armament_in_flight_notification_H__

#include "FGNotification.hpp"
#include "Emesary.hpp"
#include <string>

namespace asa::fg
{

    class ArmamentInFlightNotification : public FGNotification
    {
    public:
        ArmamentInFlightNotification(int typeId, double time);
        ArmamentInFlightNotification(int typeId, geoCoord position, int kind, int secondaryKind, double u_fps, double heading, double pitch, std::string remoteCallsign,
                                     int flags, int uniqueIdentity, double time);
        ArmamentInFlightNotification(int typeId, std::string buf, double time);
        virtual ~ArmamentInFlightNotification(){}

        virtual size_t bufSize() const;
        virtual std::string encode();
        bool decode(std::string);
        bool isValidFunc();

        int getKind() const { return kind; }
        void setKind(int kind_) { kind = kind_; }

        int getSecondaryKind() const { return secondaryKind; }
        void setSecondaryKind(int secondaryKind_) { secondaryKind = secondaryKind_; }

        geoCoord getPosition() const { return position; }
        void setPosition(geoCoord position_) { position = position_; }

        int getUniqueIndex() const { return uniqueIndex; }
        void setUniqueIndex(int uniqueIndex_) { uniqueIndex = uniqueIndex_; }

        double getHeading() const { return heading; }
        void setHeading(double heading_) { heading = heading_; }

        double getPitch() const { return pitch; }
        void setPitch(double pitch_) { pitch = pitch_; }

        double getUFps() const { return u_fps; }
        void setUFps(double u_fps_) { u_fps = u_fps_; }

        int getIsDistinct() const { return isDistinct; }
        void setIsDistinct(int isDistinct_) { isDistinct = isDistinct_; }

        std::string getCallsign() const { return Callsign; }
        void setCallsign(const std::string &Callsign_) { Callsign = Callsign_; }

        std::string getRemoteCallsign() const { return remoteCallsign; }
        void setRemoteCallsign(const std::string &remoteCallsign_) { remoteCallsign = remoteCallsign_; }

        int getFlags() const { return flags; }
        void setFlags(int flags_) { flags = flags_; }

        int getUniqueIdentity() const { return uniqueIdentity; }
        void UniqueIdentity(int uniqueIdentity_) { uniqueIdentity = uniqueIdentity_; }

        int getMsgID() const { return msgID; }
        void setMsgID(double msgID_) { msgID = msgID_; }

        int getMsgType() const { return msgType; }
        void setMsgType(double msgType_) { msgType = msgType_; }

        double getTime() const { return time; }
        void setTime(double time_) { time = time_; }

    private:
        int msgID;
        int msgType;
        geoCoord position;
        int kind;
        int secondaryKind;
        int uniqueIndex;
        double u_fps;
        double heading;
        double pitch;
        int isDistinct;
        std::string Callsign;
        std::string remoteCallsign;
        int flags;
        int uniqueIdentity;
        bool isValid;
        double time;
    };

}

#endif /* __asa_fg_armament_in_flight_notification_H__ */