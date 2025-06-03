#include "asa/fg/Factory.hpp"
#include "asa/core/base/Log.hpp"

namespace asa::fg
{
    Factory *Factory::instance = nullptr;

    Factory::Factory() {
        // empty
    };

    Factory *Factory::getInstance()
    {
        if (instance == nullptr)
        {
            instance = new Factory();
        }
        return instance;
    }

    std::vector<FGPropertyData *> Factory::getEssentialsProps()
    {

        _motionDataEssentialProperties->properties.clear();
        char buf[MAX_PACKET_SIZE];
        memset(buf, 0xff, MAX_PACKET_SIZE);
        memcpy(buf, "\x46\x47\x46\x53\x00\x01\x00\x01\x00\x00\x00\x07\x00\x00\x02\xd1\x00\x00\x01\x00\x00\x00\x00\x00\x52\x31\x53\x4d\x52\x00\x00\x00\x41\x69\x72\x63\x72\x61\x66\x74\x2f\x66\x31\x36\x2f\x4d\x6f\x64\x65\x6c\x73\x2f\x46\x2d\x31\x36\x2e\x78\x6d\x6c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x40\xf0\x66\x79\xda\x64\xf5\x71\x3f\xb9\x99\x99\x99\x99\x99\x9a\x41\x43\xea\x64\x99\xa5\xae\xb2\xc1\x30\xb4\xb2\x5c\xe6\x2c\x56\x41\x55\xbc\xc6\x32\x0d\x32\xbe\xc0\x3b\x08\x83\x3f\x6f\x60\xa7\x3f\x20\x52\x09\x43\xd1\x5a\xd1\x3f\x55\x24\x77\x40\x72\x8e\xd2\xb9\x6a\x66\x2d\x38\x6f\x42\x81\xb9\x57\x57\xf6\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1f\xac\xe0\x02\x00\x0a\x00\x02\x00\x64\xf4\x1c\x00\x65\xf4\x17\x00\x66\x01\x83\x00\x67\xff\xe9\x00\x68\x00\x00\x00\x69\x00\x00\x00\x6a\x00\x00\x00\x6e\x00\x00\x00\x70\x00\x00\x00\xc8\x00\x00\x00\xc9\x00\x00\x00\xd2\x00\x00\x00\xd3\x00\x00\x00\xdc\x00\x00\x00\xdd\x00\x00\x00\xe6\x00\x00\x00\xe7\x00\x00\x00\xf0\x00\x00\x00\xf1\x00\x00\x01\x2c\x03\xe8\x01\x2d\x03\xe8\x03\x2a\x00\x00\x03\x2b\x00\x00\x03\x2c\x00\x00\x03\x2d\x00\x00\x03\x37\x00\x00\x03\xe9\x00\x00\x03\xea\x00\x00\x03\xeb\x00\x00\x03\xec\x00\x00\x03\xed\x03\xe8\x03\xee\x00\x00\x04\xb1\x00\x00\x05\xdc\x04\xb0\x00\x00\x05\xdd\xff\xff\xd8\xf1\x05\xde\x00\x00\x05\xdf\x00\x01\x00\x00\x27\xd8\x3f\x80\x00\x00\x00\x00\x27\xda\x00\x00\x00\x00\x00\x00\x27\xdb\x44\x3d\xb9\x65\x00\x00\x27\xdc\xbc\x41\xfd\x9e\x00\x00\x27\xdd\x3d\xcc\xa4\x86\x00\x00\x27\xde\xbd\xcc\xf5\x13\x00\x00\x27\xdf\xb4\x80\x00\x80\x00\x00\x27\xe0\x3f\x80\x00\x00\x00\x00\x27\xe1\x3f\x80\x00\x00\x00\x00\x27\xe2\x00\x00\x00\x00\x00\x00\x27\xe3\x00\x00\x00\x00\x00\x00\x27\xe4\x00\x00\x00\x00\x00\x00\x27\xe5\x00\x00\x00\x00\x00\x00\x27\xe6\x3b\xea\xbc\x40\x00\x00\x27\xe7\x3a\x83\x12\x6f\x00\x00\x27\xe8\x3f\x9e\x3d\x62\x00\x00\x27\xe9\x3f\x08\xc8\xaa\x00\x00\x27\xeb\x3f\x80\x00\x00\x00\x00\x27\xec\x3f\x80\x00\x00\x00\x00\x27\xed\x3a\x83\x12\x6f\x00\x00\x27\xee\x3c\x6a\xbc\x40\x00\x00\x28\x3e\x00\x00\x00\x00\x00\x00\x28\x45\x00\x00\x00\x05\x00\x00\x28\x46\x00\x00\x00\x02\x04\x4d\x00\x07\x39\x33\x2d\x30\x35\x35\x33\x00\x00\x04\xb0\x00\x00\x00\x00\x00\x00\x05\x78\x00\x00\x00\x00\x05\xe0\x00\x00\x05\xe1\xd8\xf1\x27\x12\x00\x05\x48\x65\x6c\x6c\x6f\x27\x78\x00\x03\x38\x38\x38\x27\x79\x00\x0f\x30\x37\x2d\x2d\x2b\x2b\x30\x30\x2d\x2d\x45\x6d\x70\x74\x79\x00\x00\x27\x7a\x00\x00\x00\x00\x27\x7b\x00\x03\x83\x75\xe1\x00\x00\x27\x7c\x00\x00\x00\x00\x29\x09\x00\x64\x29\x0a\x00\x64\x29\x0b\x00\x00\x2e\xd6\x00\x01\x32\xc8\x02\x04\x00\x00\x32\xc9\x00\x00\x00\x00\x32\xca\x00\x00\x00\x00\x2a\xf8\x00\x00\x00\x06\x00\x00\x2b\x20\x00\x00\x67\x00", 721);
        asa::fg::MsgBuf msg;
        memcpy(msg.Msg, buf, sizeof(msg.Msg));
        if (!ProcessPosMsg(msg, _motionDataEssentialProperties))
        {
            std::cerr << "Error processing message" << std::endl;
        }

        _motionDataEssentialProperties->properties.push_back(FactoryEmptyNotification(17));
        _motionDataEssentialProperties->properties.push_back(FactoryEmptyNotification(18));
        _motionDataEssentialProperties->properties.push_back(FactoryEmptyNotification(19));

        return _motionDataEssentialProperties->properties;
    }

    FGExternalMotionData *Factory::HeaderAndAircraftPosition(double time, double lag,
                                                             double lat, double lon, double alt,
                                                             float heading, float pitch, float roll,
                                                             double linearV_u, double linearV_v, double linearV_w,
                                                             double angularV_p, double angularV_q, double angularV_r,
                                                             double linearAcc_u, double linearAcc_v, double linearAcc_w,
                                                             double angularAcc_p, double angularAcc_q, double angularAcc_r)
    {
        // lat               [DEG]
        // long              [DEG]
        // alt               [FT]
        // heading           [DEG]
        // pitch             [DEG]
        // roll              [DEG]
        // linearV_u (body)  [m/s]
        // linearV_v (body)  [m/s]
        // linearV_w (body)  [m/s]
        // angularV_p (body) [rad/s]
        // angularV_q (body) [rad/s]
        // angularV_r (body) [rad/s]

        FGExternalMotionData *motionInfo = new FGExternalMotionData;
        motionInfo->time = time;
        motionInfo->lag = lag; // lag = 1.0 / txRateHz
        // first the aprioriate structure for the geodetic one
        SGGeod geod = SGGeod::fromDegFt(lon, lat, alt);
        // Convert to cartesion coordinate
        motionInfo->position = SGVec3d::fromGeod(geod);

        SGQuatf qEc2Hl = SGQuatf::fromLonLatDeg((float)lon, (float)lat);
        // The orientation wrt the horizontal local frame
        SGQuatf hlOr = SGQuatf::fromYawPitchRollDeg(heading, pitch, roll);
        // The orientation of the vehicle wrt the earth centered frame
        motionInfo->orientation = qEc2Hl * hlOr;
        // velocities
        motionInfo->linearVel = SG_FEET_TO_METER * SGVec3f(linearV_u, linearV_v, linearV_w);
        motionInfo->angularVel = SGVec3f(angularV_p, angularV_q, angularV_r);
        // accels
        motionInfo->linearAccel = SGVec3f::zeros();
        motionInfo->angularAccel = SGVec3f::zeros();
        // add essentialsProps
        std::vector<FGPropertyData *> props = Factory::getEssentialsProps();
        motionInfo->properties.assign(props.begin(), props.end());

        return motionInfo;
    };

    FGPropertyData *Factory::FactoryMessage(std::string msgData, int count, int brigdeId, int notificationTypeId)
    {
        FGPropertyData *pData = new FGPropertyData;
        std::string bridgeMessageCount = BinaryAsciiTransfer::encodeInt(count, 4);
        std::string bridgeMessageNotificationTypeId = BinaryAsciiTransfer::encodeInt(notificationTypeId, 1);
        int BrigdeId = EMESARYBRIDGE_BASE + brigdeId;
        pData->id = BrigdeId;
        pData->type = simgear::props::STRING;
        std::string msg = seperatorChar + bridgeMessageCount + seperatorChar + bridgeMessageNotificationTypeId + seperatorChar + msgData + messageEndChar;
        pData->string_value = new char[msg.length() + 1];
        std::strcpy(pData->string_value, msg.c_str());
        return pData;
    };

    FGPropertyData *Factory::FactoryArmamentHitNotification(int kind, int secondaryKind, double relativeAltitude, double distance, double bearing,
                                                            std::string remoteCallsign, int uniqueIdentity)
    {
        FGPropertyData *pData = new FGPropertyData;
        Factory *fac = asa::fg::Factory::getInstance();
        ArmamentHitNotification notification = ArmamentHitNotification(ARMAMENT_HIT_NOTIFICATION_ID, kind, secondaryKind, relativeAltitude, distance, bearing, remoteCallsign, uniqueIdentity);
        notification.setMsgID(fac->GetCountHit());
        std::string msgData = notification.encode();
        pData = fac->FactoryMessage(msgData, notification.getMsgID(), ARMAMENT_HIT_NOTIFICATION_BRIGDE, ARMAMENT_HIT_NOTIFICATION_ID);
        fac->IncrementCountHit();
        return pData;
    };

    FGPropertyData *Factory::FactoryEmptyNotification(int brigdeId)
    {
        FGPropertyData *pData = new FGPropertyData;
        int BrigdeId = EMESARYBRIDGE_BASE + brigdeId;
        pData->id = BrigdeId;
        pData->type = simgear::props::STRING;
        pData->string_value = new char[1];
        pData->string_value[0] = '\0';
        return pData;
    }

    FGPropertyData *Factory::FactoryArmamentInFlightNotification(geoCoord position, int kind, int secondaryKind, double u_fps, double heading, double pitch, std::string remoteCallsign,
                                                                 int flags, int uniqueIdentity, double time)
    {
        FGPropertyData *pData = new FGPropertyData;
        Factory *fac = asa::fg::Factory::getInstance();
        ArmamentInFlightNotification notification = ArmamentInFlightNotification(ARMAMENT_IN_FLIGHT_NOTIFICATION_ID, position, kind, secondaryKind, u_fps, heading, pitch, remoteCallsign, flags, uniqueIdentity, time);
        notification.setMsgID(fac->GetCountArmamentInFlight());
        std::string msgData = notification.encode();
        pData = fac->FactoryMessage(msgData, notification.getMsgID(), ARMAMENT_IN_FLIGHT_NOTIFICATION_BRIGDE, ARMAMENT_IN_FLIGHT_NOTIFICATION_ID);
        fac->IncrementCountArmamentInFlight();
        return pData;
    };

    FGPropertyData *Factory::FactoryObjectInFlightNotification(geoCoord position, int kind, int secondaryKind, int uniqueIdentity)
    {
        FGPropertyData *pData = new FGPropertyData;
        Factory *fac = asa::fg::Factory::getInstance();
        ObjectInFlightNotification notification = ObjectInFlightNotification(OBJECT_IN_FLIGHT_NOTIFICATION_ID, position, kind, secondaryKind, uniqueIdentity);
        std::string msgData = notification.encode();
        pData = fac->FactoryMessage(msgData, fac->GetCountObjectInFlight(), OBJECT_IN_FLIGHT_NOTIFICATION_BRIGDE, OBJECT_IN_FLIGHT_NOTIFICATION_ID);
        fac->IncrementCountObjectInFlight();
        return pData;
    };

    FGPropertyData *Factory::FactoryStaticNotification(geoCoord position, int kind, int secondaryKind, double heading, int flags1, int flags2, int uniqueIdentity)
    {
        FGPropertyData *pData = new FGPropertyData;
        Factory *fac = asa::fg::Factory::getInstance();
        StaticNotification notification = StaticNotification(OBJECT_IN_FLIGHT_NOTIFICATION_ID, position, kind, secondaryKind, heading, flags1, flags2, uniqueIdentity);
        std::string msgData = notification.encode();
        pData = fac->FactoryMessage(msgData, fac->GetCountStatic(), STATIC_NOTIFICATION_BRIGDE, STATIC_NOTIFICATION_ID);
        fac->IncrementCountStatic();
        return pData;
    };
}
