#ifndef __asa_fg_fgnetio_H__
#define __asa_fg_fgnetio_H__

#include "mixr/interop/dis/NetIO.hpp"
#include "mixr/interop/dis/structs.hpp"
#include "asa/fg/Xdrclient.hpp"
#include "asa/fg/ArmamentHitNotification.hpp"
#include "asa/fg/ArmamentInFlightNotification.hpp"
#include <queue>
#include <map>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <list>

#include <iostream>
#include <fstream>  
#include <chrono>
#include <string>

#define IDFGSTART 50000
namespace asa::fg
{
    struct DetonationInfo
    {
        int firingEntityID;
        int detonationResult;
    };
    struct PDUCache
    {
        uint8_t PDUType;
        char data[mixr::dis::NetIO::MAX_PDU_SIZE];
    };

    struct ArmamentInFlightData
    {
        ArmamentInFlightData(const std::string sourceCallSign,
                             const std::string missileName,
                             ArmamentInFlightNotification *afn,
                             SGVec3f linearVel,
                             SGVec3f linerAcc,
                             SGVec3f angVel) : sourceCallSign(sourceCallSign),
                                               missileName(missileName),
                                               afn(afn),
                                               aircraft(linearVel, linerAcc, angVel) {}
        ~ArmamentInFlightData()
        {
            if (afn != nullptr)
            {
                delete afn;
            }
        }

        std::string sourceCallSign;
        std::string missileName;
        ArmamentInFlightNotification *afn = nullptr;
        struct Aircraft
        {
            Aircraft(SGVec3f linearVel, SGVec3f linerAcc, SGVec3f angVel) : linearVel(linearVel), linerAcc(linerAcc), angVel(angVel) {}
            SGVec3d linearVel;
            SGVec3d linerAcc;
            SGVec3d angVel;
        } aircraft;
    };

    struct ArmamentHitData
    {
        ArmamentHitData(const std::string sourceCallSign, const std::string missileName, ArmamentHitNotification *ahn) : sourceCallSign(sourceCallSign), missileName(missileName), ahn(ahn) {}
        std::string sourceCallSign;
        std::string missileName;
        ArmamentHitNotification *ahn = nullptr;
        ~ArmamentHitData()
        {
            if (ahn != nullptr)
            {
                delete ahn;
                ahn = nullptr;
            }
        }
    };
    struct MissileData
    {
        double time;
        SGVec3<double> pos;
        SGVec3<double> linearVel;
    };

    class AsaFGNetIO : public mixr::dis::NetIO
    {
        DECLARE_SUBCLASS(AsaFGNetIO, mixr::dis::NetIO)

    public:
        AsaFGNetIO();
        // Sends a packet (PDU) to the network  (converte antes para o formato FG)
        virtual bool sendData(const char *const packet, const int size) override;
        // Receives a packet (PDU) from the network (converte antes do formato FG)
        virtual int recvData(char *const packet, const int maxSize) override;

        mixr::interop::Nib *createNewOutputNib(mixr::models::Player *const player) override;
        bool initNetwork();
        
        std::ofstream csvFile;
        inline void logToCSV(const std::string& filepath, const std::string& label, double var1, const std::string& callSign);
        bool sendEntityState(const mixr::dis::EntityStatePDU *esp);
        bool sendKill(const mixr::dis::DetonationPDU *esp);
        bool sendFire(const mixr::dis::FirePDU *esp);

        void reset() override;

    private:
        
        int getNextPDU(char *const packet);
        uint16_t getFGID(std::string uniqueName);
        uint16_t getNextEventNumber();
        uint16_t getFGApplicationID(const std::string callSign);

        void recvThread();
        SGVec3f bodyToECEF(SGVec3f bodyV, float psi, float theta, float phi);
        PDUCache *positionToPDU(const std::string callSign, const FGExternalMotionData &motionInfo);
        PDUCache *armamentInFlightToPDU(ArmamentInFlightData &afData);
        PDUCache *armamentHitToPDU(const ArmamentHitData &ahData);

        asa::fg::XDRClient *client;
        std::string ip;
        int port;
        double maxTimeDr;

        std::queue<PDUCache *> pduQueue;
        std::unordered_map<int, DetonationInfo> detonationInfo;
        std::unordered_map<int, std::queue<FGPropertyData *>> armamentInFligthPropertyData;
        std::unordered_map<int, std::queue<FGPropertyData *>> armamentHitPropertyData;

        std::mutex newFGData_m;

        std::string uniqueFGName(mixr::models::Player *player);

        bool setSlotFgmsIP(mixr::base::String *const msg);
        bool setSlotFgmsPort(mixr::base::Number *const msg);
        bool setSlotMaxTimeDR(mixr::base::Number *const msg);
        bool setSlotBlueIffCode(mixr::base::String *const msg);
        bool setSlotRedIffCode(mixr::base::String *const msg);
        std::thread *receiveThread;
        bool running;
        std::unordered_map<std::string, uint16_t> fgIDs;
        std::unordered_map<std::string, uint16_t> fgAppIDs;

        std::mutex nextFGID_m;
        uint16_t nextFGID;
        std::mutex posBuf_m;
        std::mutex nextEventNumber_m;
        uint16_t nextEventNumber;
        uint16_t nextFGAppID; // inicia com AppID + 1
        std::mutex nextFGAppID_m;

        std::string blueIffCode;
        std::string redIffCode;

        std::unordered_map<std::string, asa::fg::FGExternalMotionData *> posBuf;

        std::unordered_map<std::string, MissileData *> armamentLastData;

        std::unordered_map<unsigned short, bool> killedPlayers; // <player_id, delete_requested?>

        std::unordered_map<std::string, ArmamentInFlightData *> bridge18Buf;
        std::unordered_map<std::string, int> lastProcIdBrid18;
        std::unordered_map<std::string, ArmamentHitData *> bridge19Buf;
        std::unordered_map<std::string, int> lastProcIdBrid19;

        std::unordered_map<std::string, int> callSignId;
    };
}

#endif /* __asa_fg_fgnetio_H__ */