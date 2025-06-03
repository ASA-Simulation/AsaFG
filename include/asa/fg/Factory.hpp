#ifndef __asa_fg_factory_H__
#define __asa_fg_factory_H__

#include "Mpmessages.hpp"
#include "Processmsg.hpp"
#include "Emesary.hpp"
#include "ArmamentHitNotification.hpp"
#include "StaticNotification.hpp"
#include "ArmamentInFlightNotification.hpp"
#include "ObjectInFlightNotification.hpp"
#include "Xdrclient.hpp"

// #==================================================================
// #                       Notification Kinds
// #==================================================================
// var CREATE = 1;
// var MOVE = 2;
// var DESTROY = 3;
// var IMPACT = 4;
// var REQUEST_ALL = 5;

namespace asa::fg
{
    const int   ARMAMENT_HIT_NOTIFICATION_ID = 19;
    const int   ARMAMENT_IN_FLIGHT_NOTIFICATION_ID = 21;
    const int   OBJECT_IN_FLIGHT_NOTIFICATION_ID = 22;
    const int   STATIC_NOTIFICATION_ID = 25;

    const int   ARMAMENT_HIT_NOTIFICATION_BRIGDE = 19;
    const int   ARMAMENT_IN_FLIGHT_NOTIFICATION_BRIGDE = 18;
    const int   OBJECT_IN_FLIGHT_NOTIFICATION_BRIGDE = 17;
    const int   STATIC_NOTIFICATION_BRIGDE = 19;

    enum Type {
    NONE = 0, /**< The node hasn't been assigned a value yet. */
    ALIAS, /**< The node "points" to another node. */
    BOOL,
    INT,
    LONG,
    FLOAT,
    DOUBLE,
    STRING,
    UNSPECIFIED,
    EXTENDED, /**< The node's value is not stored in the property;
               * the actual value and type is retrieved from an
               * SGRawValue node. This type is never returned by @see
               * SGPropertyNode::getType.
               */
    // Extended properties
    VEC3D,
    VEC4D
    };

    const char seperatorChar = '!';
    const char messageEndChar = '~';
    const int startMessageCount = 12;
    const int defaultMessageLifetimeSeconds = 10;

    class Factory
    {
        public:

            
            FGExternalMotionData* HeaderAndAircraftPosition(double time, double lag, 
                                                            double lat, double lon, double alt,
                                                            float heading, float pitch, float roll,
                                                            double linearV_u, double linearV_v, double linearV_w,
                                                            double angularV_p, double angularV_q, double angularV_r,
                                                            double linearAcc_u, double linearAcc_v, double linearAcc_w,
                                                            double angularAcc_p, double angularAcc_q, double angularAcc_r);
            FGPropertyData* FactoryArmamentHitNotification(int kind, int secondaryKind, double relativeAltitude, double distance, double bearing,
                                                    std::string remoteCallsign, int uniqueIdentity);
            FGPropertyData* FactoryArmamentInFlightNotification(geoCoord position, int kind, int secondaryKind, double u_fps, double heading, double pitch, std::string remoteCallsign,
                                                        int flags,int uniqueIdentity, double time);
            FGPropertyData* FactoryEmptyNotification(int brigdeId);
            FGPropertyData* FactoryObjectInFlightNotification(geoCoord position, int kind, int secondaryKind, int uniqueIdentity);
            FGPropertyData* FactoryStaticNotification(geoCoord position, int kind, int secondaryKind, double heading, int flags1, int flags2, int uniqueIdentity);

            void IncrementCountHit(){countHit += 1;};
            int GetCountHit(){return countHit;};

            void IncrementCountArmamentInFlight(){countArmamentInFlight += 1;};
            int GetCountArmamentInFlight(){return countArmamentInFlight;};

            void IncrementCountObjectInFlight(){countObjectInFlight += 1;};
            int GetCountObjectInFlight(){return countObjectInFlight;};

            void IncrementCountStatic(){countStatic += 1;};
            int GetCountStatic(){return countStatic;};

            static Factory* getInstance();

            std::vector<FGPropertyData *> getEssentialsProps();
            
        
        private:
            Factory();
            static Factory* instance;
            int countHit = startMessageCount;
            int countArmamentInFlight = startMessageCount;
            int countObjectInFlight = startMessageCount;
            int countStatic = startMessageCount;
            FGExternalMotionData* _motionDataEssentialProperties = new FGExternalMotionData;

            FGPropertyData* FactoryMessage(std::string msgData, int count, int brigdeId, int notificationTypeId);
            

    };

}

#endif /* __asa_fg_factory_H__ */