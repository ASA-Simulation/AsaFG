#include "asa/fg/AsaFGNetIO.hpp"
#include "asa/fg/Factory.hpp"
#include "asa/fg/Utils.hpp"
#include "asa/fg/Mpmessages.hpp"
#include "asa/models/worldmodel/AsaWorldModel.hpp"
#include "asa/core/base/Log.hpp"
#include "asa/core/base/Utils.hpp"

#include "asa/core/base/Utils.hpp"

#include "mixr/interop/dis/NetIO.hpp"
#include "mixr/interop/dis/Nib.hpp"
#include "mixr/interop/dis/Ntm.hpp"
#include "mixr/interop/dis/EmissionPduHandler.hpp"
#include "mixr/interop/dis/pdu.hpp"
#include "mixr/base/util/system_utils.hpp"
#include "mixr/base/network/NetHandler.hpp"
#include "mixr/base/String.hpp"
#include "mixr/base/numeric/Number.hpp"
#include "mixr/simulation/Station.hpp"
#include "mixr/simulation/Simulation.hpp"
#include "mixr/models/player/weapon/Missile.hpp"
#include "mixr/models/Track.hpp"
#include "mixr/models/player/air/AirVehicle.hpp"
#include "mixr/base/util/nav_utils.hpp"
#include "mixr/base/units/angle_utils.hpp"
#include "mixr/base/PairStream.hpp"
#include "asa/models/player/asa/weapon/AsaAbstractRfMissile.hpp"

#include <simgear/math/sg_geodesy.hxx>

#include <iostream>
#include <fstream>
#include <chrono>
#include <string>

namespace asa::fg
{

    // clang-format off
    IMPLEMENT_SUBCLASS(AsaFGNetIO, "AsaFGNetIO")
    
    BEGIN_SLOTTABLE(AsaFGNetIO)
        "ip",            // 1) fgms server ip
        "port",      // 2) fgms server port
        "maxTimeDR", // 3) max time for dead reckoning for heartbeat
        "blueIffCode", // 4) Red Iff Code (Flight Gear M4 code)
        "redIffCode", // 5) Red Iff Code (Flight Gear M4 code)
    END_SLOTTABLE(AsaFGNetIO)

    BEGIN_SLOT_MAP(AsaFGNetIO)
        ON_SLOT(1, setSlotFgmsIP, mixr::base::String)
        ON_SLOT(2, setSlotFgmsPort, mixr::base::Number)
        ON_SLOT(3, setSlotMaxTimeDR, mixr::base::Number)
        ON_SLOT(4, setSlotBlueIffCode, mixr::base::String)
        ON_SLOT(5, setSlotRedIffCode, mixr::base::String)
    END_SLOT_MAP()

   
    

    AsaFGNetIO::AsaFGNetIO() : client(nullptr), receiveThread(nullptr), running(true), nextFGID(IDFGSTART), nextEventNumber(1), nextFGAppID(getApplicationID() + 1)
    {
        STANDARD_CONSTRUCTOR()
    }

    // clang-format on
    void AsaFGNetIO::deleteData()
    {
        BaseClass::deleteData();
        running = false;
        if (client != nullptr)
        {
            delete client;
            client = nullptr;
        }
        if (receiveThread != nullptr)
        {
            if (receiveThread->joinable())
            {
                receiveThread->join();
                delete receiveThread;
                receiveThread = nullptr;
            }
        }
    }

    // Função para registrar dados no CSV
    void AsaFGNetIO::logToCSV(const std::string &filepath, const std::string &label, double var1, const std::string &callSign)
    {
        auto t = asa::core::Utils::microsSinceEpoch();

        std::ofstream file(filepath, std::ios::app); // Modo append

        if (!file.is_open())
        {
            std::cerr << "Erro ao abrir o arquivo CSV!" << std::endl;
            return;
        }
        // auto now = std::chrono::steady_clock::now();
        // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        file << label << "," << var1 << "," << callSign << "," << t << std::endl;
        file.close();
    }

    void AsaFGNetIO::copyData(const AsaFGNetIO &org, const bool cc)
    {
        BaseClass::copyData(org);
        ip = org.ip;
        port = org.port;
        maxTimeDr = org.maxTimeDr;
        fgIDs.insert(org.fgIDs.begin(), org.fgIDs.end());
        nextFGID = org.nextFGID;
    }

    bool AsaFGNetIO::initNetwork()
    {
        client = new asa::fg::XDRClient(ip.c_str(), port);
        receiveThread = new std::thread(&asa::fg::AsaFGNetIO::recvThread, this);
        return BaseClass::initNetwork();
    }

    void AsaFGNetIO::recvThread()
    {

        /*
            Todas as mensagens recebidas possuem posição e são colocadas na fila de posBuf (sobrescrevendo a posicao anterior do player, se tiver)
            - A mensagem que possue bridge 18, é armazenada na bridge18Buf
            - A mensagem que possue bridge 19, é armazenada na bridge19Buf
        */

        MsgBuf msg;
        T_MsgHdr *msgHdr;
        while (running)
        {
            if (client->recvData(&msg))
            {
                msgHdr = msg.msgHdr();
                std::string callSign(msgHdr->Callsign);
                if (!callSign.length())
                {
                    ASA_LOG(warning) << "Ignoring package with invalid CallSign";
                    continue;
                }
                // As mensagens enviadas por outra aplicacao AsaFG sao descartadas
                // (O Reply port é ignorado pelo FG, por isso usamos para fazer essa diferenciação do que é pacote vindo do FG ou do ASA)
                if (msgHdr->ReplyPort != REPLY_PORT_DFT)
                {

                    std::unique_lock<std::mutex> lock(posBuf_m);

                    FGExternalMotionData *emd = new FGExternalMotionData();

                    if (!ProcessPosMsg(msg, emd))
                    {
                        ASA_LOG(warning) << "Error processing FG message: " << callSign;
                        delete emd;
                        continue;
                    }

                    // se ja existe mensagem no posBuf, remove para colocar a nova
                    if (posBuf.find(callSign) != posBuf.end())
                    {
                        delete posBuf[callSign];
                        posBuf.erase(callSign);
                    }

                    uint16_t playerID = getFGID(callSign);

                    if (killedPlayers.find(playerID) == killedPlayers.end()) // se esse FGPlayer nao foi morto por um player do ASA
                    {
                        posBuf[callSign] = emd;
                    }

                    // procura as properties 18 e 19 e poe nas respectivas filas
                    for (auto prop : emd->properties)
                    {
                        switch (prop->id)
                        {
                        case EMESARYBRIDGE_BASE + 18:
                        {
                            // check if message is empty
                            if (prop->string_value[0] == 0)
                            {
                                continue;
                            }

                            ArmamentInFlightNotification *afn = new ArmamentInFlightNotification(ARMAMENT_IN_FLIGHT_NOTIFICATION_ID, prop->string_value, emd->time);

                            std::string missileName = std::string(callSign).append("_").append(std::to_string(afn->getUniqueIdentity()));

                            if (lastProcIdBrid18.find(missileName) != lastProcIdBrid18.end())
                            {
                                if ((afn->getMsgID() <= lastProcIdBrid18.at(missileName)) || (afn->getKind() != MOVE))
                                {
                                    // Essa mensagem já foi recebida anteriormente ou é mais antiga do que a armazenada no buffer.
                                    // Removo a nova mensagem e mantenho a mensagem já armazenada
                                    delete afn;
                                    break;
                                }
                                else
                                {
                                    // Nova mensagem recebida, removo a antiga para guardar a nova
                                    delete bridge18Buf[missileName];
                                    bridge18Buf.erase(missileName);
                                }
                            }
                            bridge18Buf[missileName] = new ArmamentInFlightData(callSign, missileName, afn, emd->linearVel, emd->linearAccel, emd->angularVel);
                            lastProcIdBrid18[missileName] = afn->getMsgID();
                            break;
                        }
                        case EMESARYBRIDGE_BASE + 19:
                        {

                            // check if message is empty
                            if (prop->string_value[0] == 0)
                            {
                                continue;
                            }

                            ArmamentHitNotification *ahn = new ArmamentHitNotification(ARMAMENT_HIT_NOTIFICATION_ID, prop->string_value);

                            if (ahn->getRemoteCallsign().length() == 0 || (ahn->getMsgType() != ARMAMENT_HIT_NOTIFICATION_ID && ahn->getMsgType() != STATIC_NOTIFICATION_ID))
                            {
                                continue;
                            }

                            std::string missileName = std::string(callSign).append("_").append(std::to_string(ahn->getUniqueIdentity()));

                            if (lastProcIdBrid19.find(missileName) != lastProcIdBrid19.end())
                            {
                                if ((ahn->getMsgID() <= lastProcIdBrid19.at(missileName)))
                                {
                                    // Essa mensagem já foi recebida anteriormente ou é mais antiga do que a armazenada no buffer.
                                    // Removo a nova mensagem e mantenho a mensagem já armazenada
                                    delete ahn;
                                    break;
                                }
                                else
                                {
                                    // Nova mensagem recebida, removo a antiga para guardar a nova
                                    delete bridge19Buf[missileName];
                                    bridge19Buf.erase(missileName);
                                }
                            }
                            bridge19Buf[missileName] = new ArmamentHitData(callSign, missileName, ahn);
                            lastProcIdBrid19[missileName] = ahn->getMsgID();
                            break;
                        }
                        }
                    }
                }
            }
        }
        ASA_LOG(trace) << "Parando de receber dados da rede do FG" << std::endl;
    }

    bool AsaFGNetIO::sendEntityState(const mixr::dis::EntityStatePDU *esp)
    {
        // Get PDU IDs
        unsigned short playerId{esp->entityID.ID};
        unsigned short site{esp->entityID.simulationID.siteIdentification};
        unsigned short app{esp->entityID.simulationID.applicationIdentification};

        mixr::interop::Nib *testNib{findDisNib(playerId, site, app, OUTPUT_NIB)};

        if (testNib == nullptr)
        {
            return false;
        }
        mixr::models::Player *player = testNib->getPlayer();

        char ret[MAX_PACKET_SIZE];
        memset(ret, 0xff, MAX_PACKET_SIZE);
        std::string model = "Aircraft/f16/Models/F-16.xml";
        std::stringstream ss;
        ss << player->getName()->getString() << "_" << esp->entityID.ID;

        if (callSignId.find(player->getName()->getString()) == callSignId.end())
        {
            std::string p = player->getName()->getString();
            callSignId[p.substr(0, 7)] = player->getID();
        }
        // Se o player for um missil
        if (player->isMajorType(mixr::models::Player::WEAPON))
        {
            double MissileLat = player->getLatitude();
            double MissileLon = player->getLongitude();
            double MissileAlt = player->getAltitude();
            const char *call_sign = "";

            mixr::models::Missile *w = reinterpret_cast<mixr::models::Missile *>(player);
            asa::models::AsaAbstractRfMissile *wAsa = reinterpret_cast<asa::models::AsaAbstractRfMissile *>(player);
            mixr::models::Track *track = w->getTargetTrack();
            mixr::models::Player *target = nullptr;

            if (track != nullptr)
            {
                target = track->getTarget();
                if (target != nullptr)
                {
                    call_sign = target->getName()->getCopyString();
                }
            }

            mixr::models::Player *launcher = w->getLaunchVehicle();
            int firingEntityID = 0;
            if (launcher != nullptr)
            {
                firingEntityID = launcher->getID();
            }

            asa::fg::FGPropertyData *msg = nullptr;
            asa::fg::Factory *fac = asa::fg::Factory::getInstance();

            if (detonationInfo.find(playerId) != detonationInfo.end())
            {
                // ArmamentHitNotification
                double targetLat;
                double targetLon;
                double targetAlt;
                if (target != nullptr)
                {
                    targetLat = target->getLatitude();
                    targetLon = target->getLongitude();
                    targetAlt = target->getAltitude();

                    if (killedPlayers.find(target->getID()) == killedPlayers.end())
                    {
                        target->event(KILL_EVENT, w->getLaunchVehicle());
                        target->setMode(mixr::simulation::AbstractPlayer::Mode::DELETE_REQUEST);
                        killedPlayers.insert(std::make_pair(target->getID(), false));
                        ASA_LOG(debug) << "Adicionando Player " << target->getName()->getString() << " na lista de players mortos";
                    }
                }

                int kind = 4;            // impact (1: create, 2: move, 3: destory, 4: impact, 5: request-all)
                int secondary_kind = 73; // AIM-120
                double relativeAltitude = 0.5;
                double distance{0.0};
                double bearing{0.0};
                mixr::base::nav::gll2bd(MissileLat, MissileLon, targetLat, targetLon, &bearing, &distance);
                distance = distance * mixr::base::distance::NM2M;
                msg = fac->FactoryArmamentHitNotification(kind, secondary_kind, relativeAltitude, distance, bearing, call_sign, (int)w->getID());

                if (armamentHitPropertyData.find(firingEntityID) != armamentHitPropertyData.end())
                {
                    armamentHitPropertyData.at(firingEntityID).push(msg);
                }
                else
                {
                    std::queue<FGPropertyData *> propData;
                    propData.push(msg);
                    armamentHitPropertyData.insert(std::make_pair(firingEntityID, propData));
                }
            }
            else
            {
                // ArmamentInFligthNotification
                asa::fg::geoCoord coord{MissileLat, MissileLon, MissileAlt};
                int kind = 2;            // move
                int secondary_kind = 73; // AIM-120
                double u_fps = player->getVelocityBody()[0];
                double heading = player->getHeadingD();
                double pitch = player->getPitchD();
                int flag = 0;
                if (wAsa->getSeekerOn())
                {
                    flag = flag + 1;
                }
                if (wAsa->getThrust())
                {
                    flag = flag + 2;
                }

                int unique_id = playerId;
                msg = fac->FactoryArmamentInFlightNotification(coord, kind, secondary_kind, u_fps, heading, pitch, call_sign,
                                                               flag, unique_id, 0.);

                if (armamentInFligthPropertyData.find(firingEntityID) != armamentInFligthPropertyData.end())
                {
                    armamentInFligthPropertyData.at(firingEntityID).push(msg);
                }
                else
                {
                    std::queue<FGPropertyData *> propData;
                    propData.push(msg);
                    armamentInFligthPropertyData.insert(std::make_pair(firingEntityID, propData));
                }
            }
        }
        else
        {
            std::string callsign = ss.str();

            asa::fg::Factory *fac = asa::fg::Factory::getInstance();

            asa::fg::FGExternalMotionData *motionInfo;

            double time = player->getSynchronizedState().getTimeExec();

            double lat = player->getLatitude();
            double lon = player->getLongitude();
            double altitude = player->getAltitudeFt();

            // angles
            mixr::base::Vec3d angles = player->getEulerAngles();
            double roll = mixr::base::angle::aepcdDeg(angles[mixr::models::Player::IROLL] * mixr::base::angle::R2DCC);
            double pitch = mixr::base::angle::aepcdDeg(angles[mixr::models::Player::IPITCH] * mixr::base::angle::R2DCC);
            double heading = mixr::base::angle::aepcdDeg(angles[mixr::models::Player::IYAW] * mixr::base::angle::R2DCC);

            // Body Velocity
            mixr::base::Vec3d body_vel = player->getVelocityBody();
            double vu = body_vel[0];
            double vv = body_vel[1];
            double vw = body_vel[2];

            // Body Angular Velocity
            mixr::base::Vec3d body_ang_vel = player->getAngularVelocities();
            double vp = body_ang_vel[0];
            double vq = body_ang_vel[1];
            double vr = body_ang_vel[2];

            motionInfo = fac->HeaderAndAircraftPosition(time, 0.1,
                                                        lat, lon, altitude,
                                                        heading, pitch, roll,
                                                        vu, vv, vw,
                                                        vp, vq, vr,
                                                        0, 0, 0,
                                                        0, 0, 0);

            if (armamentInFligthPropertyData.find(playerId) != armamentInFligthPropertyData.end())
            {
                std::queue<FGPropertyData *> &q = armamentInFligthPropertyData.at(playerId);
                if (!q.empty())
                {
                    FGPropertyData *propData = q.front();
                    q.pop();
                    asa::fg::setInFligthNotification(motionInfo, propData->string_value);
                }
                else
                {
                    armamentInFligthPropertyData.erase(playerId);
                }
            }

            if (armamentHitPropertyData.find(playerId) != armamentHitPropertyData.end())
            {
                std::queue<FGPropertyData *> &q = armamentHitPropertyData.at(playerId);
                if (!q.empty())
                {
                    FGPropertyData *propData = q.front();
                    q.pop();
                    asa::fg::setHitNotification(motionInfo, propData->string_value);
                }
                else
                {
                    armamentHitPropertyData.erase(playerId);
                }
            }

            // setando fumaça apenas para testes
            // REMOVER na versão final
            if (callsign[0] == 'B')
            {
                asa::fg::setSmokeGreen(motionInfo, 1);
                asa::fg::setStr4Iff(motionInfo, blueIffCode);
            }
            else
            {
                asa::fg::setSmokeRed(motionInfo, 1);
                asa::fg::setStr4Iff(motionInfo, redIffCode);
            }

            asa::fg::setSmoke(motionInfo, 1);

            // setando visual/pintura do aviao
            asa::fg::setLivery(motionInfo, "91-0404");

            int len = asa::fg::XDRClient::toBuf(ret, *motionInfo, asa::fg::MAX_MP_PROTOCOL_VERSION, model, 1, callsign);

            client->sendBuffer(ret, len);
        }
        return true;
    }

    bool AsaFGNetIO::sendKill(const mixr::dis::DetonationPDU *esp)
    {
        detonationInfo.insert(std::make_pair(esp->munitionID.ID, DetonationInfo{esp->firingEntityID.ID, esp->detonationResult}));
        return true;
    }

    bool AsaFGNetIO::sendFire(const mixr::dis::FirePDU *esp)
    {
        return true;
    }

    bool AsaFGNetIO::sendData(const char *const packet, const int size)
    {
        // return BaseClass::sendData(packet, size);

        const mixr::dis::PDUHeader *header{reinterpret_cast<const mixr::dis::PDUHeader *>(packet)};

        switch (header->PDUType)
        {
        case PDU_ENTITY_STATE:
        {
            mixr::dis::EntityStatePDU pPdu;
            memcpy(&pPdu, header, sizeof(mixr::dis::EntityStatePDU));

            if (mixr::base::NetHandler::isNotNetworkByteOrder())
            {
                pPdu.swapBytes();
            }
            return sendEntityState(&pPdu);
        }
        case PDU_FIRE:
        {
            mixr::dis::FirePDU fPdu;
            memcpy(&fPdu, header, sizeof(mixr::dis::FirePDU));
            if (mixr::base::NetHandler::isNotNetworkByteOrder())
            {
                fPdu.swapBytes();
            }
            return sendFire(&fPdu);
        }
        case PDU_DETONATION:
        {
            mixr::dis::DetonationPDU dPdu;
            memcpy(&dPdu, header, sizeof(mixr::dis::DetonationPDU));
            if (mixr::base::NetHandler::isNotNetworkByteOrder())
            {
                dPdu.swapBytes();
            }
            if (dPdu.detonationResult == DETONATION_RESULT_ENTITY_IMPACT)
            {
                return sendKill(&dPdu);
            }

            break;
        }

        default:
            return false;
        }
        return true;
    }

    void AsaFGNetIO::reset()
    {
        setMaxTimeDR(maxTimeDr, 255, 255);
        fgIDs.clear();
        {
            std::unique_lock<std::mutex> lock(nextFGID_m);
            nextFGID = IDFGSTART;
        }
        {
            std::unique_lock<std::mutex> lock(posBuf_m);
            for (auto f : posBuf)
            {
                delete f.second;
            }
            posBuf.clear();

            for (auto m : bridge18Buf)
            {
                delete m.second;
            }
            bridge18Buf.clear();

            for (auto f : bridge19Buf)
            {
                delete f.second;
            }
            bridge19Buf.clear();
        }
        BaseClass::reset();
    }

    SGVec3f AsaFGNetIO::bodyToECEF(SGVec3f bodyV, float psi, float theta, float phi)
    {
        std::vector<double> v_body = {bodyV[0], bodyV[1], bodyV[2]};

        // Rotation matrices as linear arrays
        std::vector<double> R_x(9, 0.0);
        setElement(R_x, 3, 3, 0, 0, 1);
        setElement(R_x, 3, 3, 1, 1, cos(phi));
        setElement(R_x, 3, 3, 1, 2, -sin(phi));
        setElement(R_x, 3, 3, 2, 1, sin(phi));
        setElement(R_x, 3, 3, 2, 2, cos(phi));

        std::vector<double> R_y(9, 0.0);
        setElement(R_y, 3, 3, 0, 0, cos(theta));
        setElement(R_y, 3, 3, 0, 2, sin(theta));
        setElement(R_y, 3, 3, 1, 1, 1);
        setElement(R_y, 3, 3, 2, 0, -sin(theta));
        setElement(R_y, 3, 3, 2, 2, cos(theta));

        std::vector<double> R_z(9, 0.0);
        setElement(R_z, 3, 3, 0, 0, cos(psi));
        setElement(R_z, 3, 3, 0, 1, -sin(psi));
        setElement(R_z, 3, 3, 1, 0, sin(psi));
        setElement(R_z, 3, 3, 1, 1, cos(psi));
        setElement(R_z, 3, 3, 2, 2, 1);

        // Combined rotation matrix
        auto R_temp = multiplyMatrices(R_z, R_y, 3, 3);
        auto R = multiplyMatrices(R_temp, R_x, 3, 3);

        // World velocity vector
        auto v_world = multiplyMatrixVector(R, v_body, 3, 3);

        return SGVec3f(v_world[0], v_world[1], v_world[2]);
    }

    PDUCache *AsaFGNetIO::positionToPDU(const std::string callSign, const FGExternalMotionData &motionInfo)
    {

        uint16_t playerID = getFGID(callSign);
        bool killed = killedPlayers.find(playerID) != killedPlayers.end();
        PDUCache *cache = new PDUCache();
        cache->PDUType = killed ? PDU_REMOVE_ENTITY : PDU_ENTITY_STATE;

        // criar EntityStatePDU
        mixr::dis::EntityStatePDU *pPdu = {reinterpret_cast<mixr::dis::EntityStatePDU *>(cache->data)};

        // Preenchimento dos dados de EntityStatePDU feito inspirado no método bool Nib::entityStateManager(const double curExecTime): mixr/src/interop/dis/Nib_entity_state.cpp

        // PDUHeader
        pPdu->header.protocolVersion = VERSION_7;
        pPdu->header.exerciseIdentifier = getExerciseID();
        pPdu->header.PDUType = killed ? PDU_REMOVE_ENTITY : PDU_ENTITY_STATE;
        pPdu->header.protocolFamily = mixr::dis::NetIO::PDU_FAMILY_ENTITY_INFO;
        pPdu->header.timeStamp = timeStamp();
        pPdu->header.length = sizeof(mixr::dis::EntityStatePDU);
        pPdu->header.status = 0;
        pPdu->header.padding = 0;

        // EntityIdentifierDIS
        pPdu->entityID.simulationID.siteIdentification = getSiteID();
        pPdu->entityID.simulationID.applicationIdentification = getFGApplicationID(callSign);
        // Obtem o ID associado ao callSign ou cria um novo (decremental, partindo de IDFGSTART)

        pPdu->entityID.ID = getFGID(callSign);

        switch (callSign.at(0))
        {
        case 'b':
        case 'B':
            pPdu->forceID = mixr::dis::NetIO::ForceEnum::FRIENDLY_FORCE;
            break;
        case 'r':
        case 'R':
            pPdu->forceID = mixr::dis::NetIO::ForceEnum::OPPOSING_FORCE;
            break;
        default:
            pPdu->forceID = mixr::dis::NetIO::ForceEnum::NEUTRAL_FORCE;
            break;
        }

        pPdu->numberOfArticulationParameters = 0;
        // EntityType (F-16C)
        // ( DisNtm  disEntityType: [ 1  2  225  1  3  3  0 ]    template: ( Aircraft type: "F16C" ) )
        // AsaFighter 1  2  225  1  3  9  0
        pPdu->entityType.kind = 1;
        pPdu->entityType.domain = 2;
        pPdu->entityType.country = 225;
        pPdu->entityType.category = 1;
        pPdu->entityType.subcategory = 3;
        pPdu->entityType.specific = 9;
        pPdu->entityType.extra = 0;

        // alternativeType (usa o mesmo que entityType: ver NetIO.hpp (dis))
        pPdu->alternativeType.kind = pPdu->entityType.kind;
        pPdu->alternativeType.domain = pPdu->entityType.domain;
        pPdu->alternativeType.country = pPdu->entityType.country;
        pPdu->alternativeType.category = pPdu->entityType.category;
        pPdu->alternativeType.subcategory = pPdu->entityType.subcategory;
        pPdu->alternativeType.specific = pPdu->entityType.specific;
        pPdu->alternativeType.extra = pPdu->entityType.extra;

        // Entity orientation (EulerAngles)
        float psi, theta, phi;
        motionInfo.orientation.getEulerRad(psi, theta, phi);

        pPdu->entityOrientation.psi = psi;
        pPdu->entityOrientation.theta = theta;
        pPdu->entityOrientation.phi = phi;

        auto linearVel = bodyToECEF(motionInfo.linearVel, psi, theta, phi);

        // Entity linear velocity (VectorDIS)
        pPdu->entityLinearVelocity.component[0] = linearVel[0];
        pPdu->entityLinearVelocity.component[1] = linearVel[1];
        pPdu->entityLinearVelocity.component[2] = linearVel[2];

        // Entity location (WorldCoordinates)
        pPdu->entityLocation.X_coord = motionInfo.position[0];
        pPdu->entityLocation.Y_coord = motionInfo.position[1];
        pPdu->entityLocation.Z_coord = motionInfo.position[2];

        pPdu->appearance = 0x0;

        // Dead reckoning algorithm
        pPdu->deadReckoningAlgorithm = static_cast<unsigned char>(mixr::interop::Nib::DeadReckoning::FVW_DRM); // padrao

        // Other parameters
        for (unsigned int i = 0; i < 15; i++)
        {
            pPdu->otherParameters[i] = 0;
        }

        // Dead reckoning information
        // Dead reckoning linear acceleration (VectorDIS)
        pPdu->DRentityLinearAcceleration.component[0] = motionInfo.linearAccel[0];
        pPdu->DRentityLinearAcceleration.component[1] = motionInfo.linearAccel[1];
        pPdu->DRentityLinearAcceleration.component[2] = motionInfo.linearAccel[2];

        // Dead reckoning angular velocity (AngularVelocityVectorDIS)
        pPdu->DRentityAngularVelocity.x_axis = motionInfo.angularVel[0];
        pPdu->DRentityAngularVelocity.y_axis = motionInfo.angularVel[1];
        pPdu->DRentityAngularVelocity.z_axis = motionInfo.angularVel[2];

        // Entity marking (EntityMarking)
        {
            const char *const pName{callSign.c_str()};
            std::size_t nameLen{std::strlen(pName)};
            for (unsigned int i = 0; i < mixr::dis::EntityMarking::BUFF_SIZE; i++)
            {
                if (i < nameLen)
                {
                    pPdu->entityMarking.marking[i] = pName[i];
                }
                else
                {
                    pPdu->entityMarking.marking[i] = '\0';
                }
            }
            pPdu->entityMarking.characterSet = 1;
        }

        // Capabilities
        pPdu->capabilites = 0x0;

        // Articulation parameters
        pPdu->numberOfArticulationParameters = 0;

        if (mixr::base::NetHandler::isNotNetworkByteOrder())
        {
            pPdu->swapBytes();
        }

        // Não trata os properties, pq as properties de interesse estao armazenadas em outras filas
        return cache;
    }

    PDUCache *AsaFGNetIO::armamentInFlightToPDU(ArmamentInFlightData &afData)
    {
        PDUCache *pduCache = new PDUCache();
        pduCache->PDUType = PDU_ENTITY_STATE;
        // criar EntityStatePDU
        mixr::dis::EntityStatePDU *pPdu = {reinterpret_cast<mixr::dis::EntityStatePDU *>(pduCache->data)};

        ArmamentInFlightNotification *afn = afData.afn;

        // PDUHeader
        // ASA AIM - 120 [ 2  1  225  1  2  9  0 ]
        pPdu->header.protocolVersion = VERSION_7;
        pPdu->header.exerciseIdentifier = getExerciseID();
        pPdu->header.PDUType = mixr::dis::NetIO::PDU_ENTITY_STATE;
        pPdu->header.protocolFamily = mixr::dis::NetIO::PDU_FAMILY_ENTITY_INFO;
        pPdu->header.timeStamp = timeStamp();
        pPdu->header.length = sizeof(mixr::dis::EntityStatePDU);
        pPdu->header.status = 0;
        pPdu->header.padding = 0;

        // EntityIdentifierDIS
        pPdu->entityID.simulationID.siteIdentification = getSiteID();
        pPdu->entityID.simulationID.applicationIdentification = getFGApplicationID(afData.sourceCallSign);

        pPdu->entityID.ID = getFGID(afData.missileName);

        pPdu->numberOfArticulationParameters = 0;

        if (afData.afn->getSecondaryKind() == 53) // tipo do secondary kind da bomba
        {
            pPdu->entityType.kind = 2;
            pPdu->entityType.domain = 9;
            pPdu->entityType.country = 225;
            pPdu->entityType.category = 1;
            pPdu->entityType.subcategory = 14;
            pPdu->entityType.specific = 99;
            pPdu->entityType.extra = 0;
        }
        else
        {
            // ASA AIM - 120
            // ( DisNtm
            //    disEntityType: [ 2  1  225  1  2  9  0 ]
            //    template: ( AsaModelsR/AsaAim120 )
            // )
            pPdu->entityType.kind = 2;
            pPdu->entityType.domain = 1;
            pPdu->entityType.country = 225;
            pPdu->entityType.category = 1;
            pPdu->entityType.subcategory = 2;
            pPdu->entityType.specific = 9;
            pPdu->entityType.extra = 0;
        }

        // alternativeType (usa o mesmo que entityType: ver NetIO.hpp (dis))
        pPdu->alternativeType.kind = pPdu->entityType.kind;
        pPdu->alternativeType.domain = pPdu->entityType.domain;
        pPdu->alternativeType.country = pPdu->entityType.country;
        pPdu->alternativeType.category = pPdu->entityType.category;
        pPdu->alternativeType.subcategory = pPdu->entityType.subcategory;
        pPdu->alternativeType.specific = pPdu->entityType.specific;
        pPdu->alternativeType.extra = pPdu->entityType.extra;

        // Entity location (WorldCoordinates)

        SGVec3<double> posCart;
        SGGeodesy::SGGeodToCart(SGGeod::fromDegM(afn->getPosition().lon, afn->getPosition().lat, afn->getPosition().alt), posCart);

        pPdu->entityLocation.X_coord = posCart[0];
        pPdu->entityLocation.Y_coord = posCart[1];
        pPdu->entityLocation.Z_coord = posCart[2];

        // Entity orientation (EulerAngles)

        SGQuatf qEc2Hl = SGQuatf::fromLonLatDeg((float)afn->getPosition().lon, (float)afn->getPosition().lat);
        SGQuatf hlOr = SGQuatf::fromYawPitchRollDeg(afn->getHeading(), afn->getPitch(), 0.);
        SGQuatf orientation = qEc2Hl * hlOr;

        float psi, theta, phi;
        orientation.getEulerRad(psi, theta, phi);
        pPdu->entityOrientation.psi = psi;
        pPdu->entityOrientation.theta = theta;
        pPdu->entityOrientation.phi = phi;

        std::vector<double> v_body = {afn->getUFps() * 0.3048, 0., 0.}; // Example body velocities along x, y, z

        // Rotation matrices as linear arrays
        std::vector<double> R_x(9, 0.0);
        setElement(R_x, 3, 3, 0, 0, 1);
        setElement(R_x, 3, 3, 1, 1, cos(phi));
        setElement(R_x, 3, 3, 1, 2, -sin(phi));
        setElement(R_x, 3, 3, 2, 1, sin(phi));
        setElement(R_x, 3, 3, 2, 2, cos(phi));

        std::vector<double> R_y(9, 0.0);
        setElement(R_y, 3, 3, 0, 0, cos(theta));
        setElement(R_y, 3, 3, 0, 2, sin(theta));
        setElement(R_y, 3, 3, 1, 1, 1);
        setElement(R_y, 3, 3, 2, 0, -sin(theta));
        setElement(R_y, 3, 3, 2, 2, cos(theta));

        std::vector<double> R_z(9, 0.0);
        setElement(R_z, 3, 3, 0, 0, cos(psi));
        setElement(R_z, 3, 3, 0, 1, -sin(psi));
        setElement(R_z, 3, 3, 1, 0, sin(psi));
        setElement(R_z, 3, 3, 1, 1, cos(psi));
        setElement(R_z, 3, 3, 2, 2, 1);

        // Combined rotation matrix
        auto R_temp = multiplyMatrices(R_z, R_y, 3, 3);
        auto R = multiplyMatrices(R_temp, R_x, 3, 3);

        // World velocity vector
        auto v_world = multiplyMatrixVector(R, v_body, 3, 3);

        // ###############################################################

        // Entity linear velocity (VectorDIS)
        // A property 18 não possui velocity e acc.
        // Os dados de posicao e velocidade anteriores são armazenados para fazer o cálculo
        // SGVec3<double> linearVel{0., 0., 0.};
        SGVec3<double> linerAcc{0., 0., 0.};

        /*
            Se houver dados antigos (a partir da segunda mensagem), são calculadas as velocidades (linear e angular) e aceleração linear.
            Se não houver dados antigos (primeira mensagem), as velocidades e aceleração são copiadas dos dados da aeronave que lançou o armamento
            TODO: usar a aceleração da aeronave ?
        */
        double now = (double)afData.afn->getTime();
        MissileData *current = new MissileData();
        current->linearVel[0] = v_world[0];
        current->linearVel[1] = v_world[1];
        current->linearVel[2] = v_world[2];
        current->time = now;

        if (armamentLastData.find(afData.missileName) != armamentLastData.end())
        {
            MissileData *old = armamentLastData[afData.missileName];
            // TODO: Verificar a unidade do time da variavel time (considerando s)
            double dt = (now - old->time);
            if (dt > 0)
            {
                linerAcc[0] = current->linearVel[0] - old->linearVel[0];
                linerAcc[1] = current->linearVel[1] - old->linearVel[1];
                linerAcc[2] = current->linearVel[2] - old->linearVel[2];
                linerAcc[0] /= dt;
                linerAcc[1] /= dt;
                linerAcc[2] /= dt;
            }
            else
            {
                // Se não houve alteração no tempo, considera a mesma velocidade linear e zero para linearAcc e angVel
                current->time = old->time;
                current->linearVel[0] = old->linearVel[0];
                current->linearVel[1] = old->linearVel[1];
                current->linearVel[2] = old->linearVel[2];
            }
            delete old;
            armamentLastData.erase(afData.missileName);
        }
        else
        {
            // Se nao existem dados anteriores, utiliza velocidade e aceleração da aeronave (usar aceleração mesmo?)

            linerAcc[0] = afData.aircraft.linerAcc[0];
            linerAcc[1] = afData.aircraft.linerAcc[1];
            linerAcc[2] = afData.aircraft.linerAcc[2];

            // Cria variavel para guardar dados para o proximo loop
        }

        current->pos = posCart;
        armamentLastData[afData.missileName] = current;

        if (((afn->getUFps() * 0.3048) < 50) && afn->getSecondaryKind() != 53)
        {
            delete pduCache;
            return nullptr;
        }

        // ###############################################################

        pPdu->entityLinearVelocity.component[0] = v_world[0];
        pPdu->entityLinearVelocity.component[1] = v_world[1];
        pPdu->entityLinearVelocity.component[2] = v_world[2];

        pPdu->appearance = 0x0;

        // Dead reckoning algorithm
        pPdu->deadReckoningAlgorithm = static_cast<unsigned char>(mixr::interop::Nib::DeadReckoning::FVW_DRM); // padrao

        // Other parameters
        for (unsigned int i = 0; i < 15; i++)
        {
            pPdu->otherParameters[i] = 0;
        }

        // Dead reckoning information
        // Dead reckoning linear acceleration (VectorDIS)
        pPdu->DRentityLinearAcceleration.component[0] = linerAcc[0];
        pPdu->DRentityLinearAcceleration.component[1] = linerAcc[1];
        pPdu->DRentityLinearAcceleration.component[2] = linerAcc[2];

        // Dead reckoning angular velocity (AngularVelocityVectorDIS)
        // TODO: mudar para heading e pitch, roll (=0)
        pPdu->DRentityAngularVelocity.x_axis = 0.;
        pPdu->DRentityAngularVelocity.y_axis = 0.;
        pPdu->DRentityAngularVelocity.z_axis = 0.;

        // Entity marking (EntityMarking)
        {
            const char *const pName{afData.missileName.c_str()};
            std::size_t nameLen{std::strlen(pName)};
            for (unsigned int i = 0; i < mixr::dis::EntityMarking::BUFF_SIZE; i++)
            {
                if (i < nameLen)
                {
                    pPdu->entityMarking.marking[i] = pName[i];
                }
                else
                {
                    pPdu->entityMarking.marking[i] = '\0';
                }
            }
            pPdu->entityMarking.characterSet = 1;
        }

        // Capabilities
        pPdu->capabilites = 0x0;

        // Articulation parameters
        pPdu->numberOfArticulationParameters = 0;

        if (mixr::base::NetHandler::isNotNetworkByteOrder())
        {
            pPdu->swapBytes();
        }
        return pduCache;
    }

    PDUCache *AsaFGNetIO::armamentHitToPDU(const ArmamentHitData &ahData)
    {
        PDUCache *pduCache = new PDUCache();
        pduCache->PDUType = PDU_DETONATION;

        ArmamentHitNotification *ahn = ahData.ahn;

        // criar EntityStatePDU
        mixr::dis::DetonationPDU *pPdu = {reinterpret_cast<mixr::dis::DetonationPDU *>(pduCache->data)};

        // PDUHeader
        pPdu->header.protocolVersion = VERSION_7;
        pPdu->header.exerciseIdentifier = getExerciseID();
        pPdu->header.PDUType = mixr::dis::NetIO::PDU_DETONATION;
        pPdu->header.protocolFamily = mixr::dis::NetIO::PDU_FAMILY_WARFARE;
        pPdu->header.timeStamp = timeStamp();
        pPdu->header.length = sizeof(mixr::dis::DetonationPDU);
        pPdu->header.status = 0;
        pPdu->header.padding = 0;

        pPdu->firingEntityID.ID = getFGID(ahData.sourceCallSign);
        pPdu->firingEntityID.simulationID.siteIdentification = getSiteID();
        pPdu->firingEntityID.simulationID.applicationIdentification = getFGApplicationID(ahData.sourceCallSign);

        pPdu->munitionID.ID = getFGID(ahData.missileName);
        pPdu->munitionID.simulationID.siteIdentification = getSiteID();
        // TODO: calcular o applicationID para cada instancia do FG (usando o callSign da aeronave)
        pPdu->munitionID.simulationID.applicationIdentification = pPdu->firingEntityID.simulationID.applicationIdentification;

        pPdu->eventID.simulationID.siteIdentification = getSiteID();
        pPdu->eventID.simulationID.applicationIdentification = pPdu->firingEntityID.simulationID.applicationIdentification;
        pPdu->eventID.eventNumber = getNextEventNumber();

        if (armamentLastData.find(ahData.missileName) != armamentLastData.end())
        {
            asa::fg::MissileData *md = armamentLastData[ahData.missileName];
            pPdu->velocity.component[0] = md->linearVel[0];
            pPdu->velocity.component[1] = md->linearVel[1];
            pPdu->velocity.component[2] = md->linearVel[2];

            pPdu->location.X_coord = md->pos[0];
            pPdu->location.Y_coord = md->pos[1];
            pPdu->location.Z_coord = md->pos[2];
        }
        else
        {
            pPdu->velocity.component[0] = 0;
            pPdu->velocity.component[1] = 0;
            pPdu->velocity.component[2] = 0;

            pPdu->location.X_coord = 0;
            pPdu->location.Y_coord = 0;
            pPdu->location.Z_coord = 0;
        }

        if (ahData.ahn->getMsgType() == 25) // tipo da mensagem da bomba
        {
            pPdu->burst.munition.kind = 2;
            pPdu->burst.munition.domain = 9;
            pPdu->burst.munition.country = 225;
            pPdu->burst.munition.category = 1;
            pPdu->burst.munition.subcategory = 14;
            pPdu->burst.munition.specific = 99;
            pPdu->burst.munition.extra = 0;
        }
        else
        {
            pPdu->burst.munition.kind = 2;
            pPdu->burst.munition.domain = 1;
            pPdu->burst.munition.country = 225;
            pPdu->burst.munition.category = 1;
            pPdu->burst.munition.subcategory = 2;
            pPdu->burst.munition.specific = 9;
            pPdu->burst.munition.extra = 0;
        }
        pPdu->burst.warhead = 0;
        pPdu->burst.fuse = 0;
        pPdu->burst.quantity = 1;
        pPdu->burst.rate = 0;
        //

        if (callSignId.find(ahn->getRemoteCallsign()) != callSignId.end())
        {
            mixr::interop::Nib *testNib{findDisNib(callSignId[ahn->getRemoteCallsign()], getSiteID(), getApplicationID(), OUTPUT_NIB)};

            if (testNib == nullptr)
            {
                ASA_LOG(warning) << "Error getting nib";
                delete pduCache;
                return nullptr;
            }

            pPdu->locationInEntityCoordinates.component[0] = ahn->getDistance() * std::cos(ahn->getBearing());
            pPdu->locationInEntityCoordinates.component[1] = ahn->getDistance() * std::sin(ahn->getBearing());
            pPdu->locationInEntityCoordinates.component[2] = ahn->getRelativeAltitude();

            pPdu->targetEntityID.ID = callSignId[ahn->getRemoteCallsign()];
            pPdu->targetEntityID.simulationID.siteIdentification = getSiteID();
            pPdu->targetEntityID.simulationID.applicationIdentification = getApplicationID();
        }
        else
        {
            pPdu->locationInEntityCoordinates.component[0] = 1.;
            pPdu->locationInEntityCoordinates.component[1] = 1.;
            pPdu->locationInEntityCoordinates.component[2] = -1.;

            pPdu->targetEntityID.ID = 0;
            pPdu->targetEntityID.simulationID.siteIdentification = getSiteID();
            pPdu->targetEntityID.simulationID.applicationIdentification = getApplicationID();
        }

        pPdu->numberOfArticulationParameters = 0;
        pPdu->padding = 0;

        pPdu->detonationResult = DETONATION_RESULT_ENTITY_IMPACT;

        if (mixr::base::NetHandler::isNotNetworkByteOrder())
        {
            pPdu->swapBytes();
        }
        return pduCache;
    }

    int AsaFGNetIO::getNextPDU(char *const packet)
    {
        if (pduQueue.size() > 0)
        {
            // ##################################### 3
            PDUCache *pduCache = pduQueue.front();
            pduQueue.pop();
            size_t s;
            switch (pduCache->PDUType)
            {
            case PDU_ENTITY_STATE:
                s = sizeof(mixr::dis::EntityStatePDU);
                break;
            case PDU_FIRE:
                s = sizeof(mixr::dis::FirePDU);
                break;
            case PDU_DETONATION:
                s = sizeof(mixr::dis::DetonationPDU);
                break;
            default:
                delete pduCache;
                return -1;
            }
            memcpy(packet, &pduCache->data, s);
            delete pduCache;
            return (int)s;
        }
        return 0;
    }

    uint16_t AsaFGNetIO::getFGID(std::string uniqueName)
    {
        if (fgIDs.find(uniqueName) == fgIDs.end())
        {
            std::unique_lock<std::mutex> lock(nextFGID_m);
            fgIDs[uniqueName] = nextFGID--;
        }
        return fgIDs[uniqueName];
    }

    uint16_t AsaFGNetIO::getNextEventNumber()
    {
        std::unique_lock<std::mutex> lock(nextEventNumber_m);
        return nextEventNumber++;
    }

    uint16_t AsaFGNetIO::getFGApplicationID(const std::string callSign)
    {
        if (fgAppIDs.find(callSign) == fgAppIDs.end())
        {
            std::unique_lock<std::mutex> lock(nextFGAppID_m);
            fgAppIDs[callSign] = nextFGAppID++;
        }
        return fgAppIDs[callSign];
    }

    int AsaFGNetIO::recvData(char *const packet, const int maxSize)
    {
        // Tipos de PDU que serao tratatos no AsaFG
        // EntityStatePDU:
        // FirePDU:
        // DetonationPDU:

        /*
            Se tiver algo na fila de PDUs, retorna o primeiro elemento
            Se nao tiver, pega as mensagens em cache e popula a fila de PDUs
            Se tiver algo na fila de PDUs, retorna o primeiro elemento
        */

        // Se tiver algo na fila de PDUs, retorna o primeiro elemento
        int size = getNextPDU(packet);
        if (size > 0)
        {
            return size;
        }

        //  Pega as mensagens em cache (bridge18, bridge18 e posicao) e popula a fila de PDUs
        {
            std::unique_lock<std::mutex> lock(posBuf_m);

            for (auto el : bridge18Buf)
            {
                PDUCache *pduCache = armamentInFlightToPDU(*el.second);

                if (pduCache != nullptr)
                {
                    pduQueue.push(pduCache);
                }
                else
                {
                    ASA_LOG(error) << "[ERROR] Error processing bridge18Buf : " << el.first << ".";
                }
                delete el.second;
            }
            // limpa o buffer para receber mais mensagens na outra thread
            bridge18Buf.clear();

            for (auto el : bridge19Buf)
            {
                PDUCache *pduCache = armamentHitToPDU(*el.second);
                if (pduCache != nullptr)
                {
                    pduQueue.push(pduCache);
                }
                else
                {
                    ASA_LOG(error) << "[ERROR] Error processing bridge19Buf : " << el.first << ".";
                }
                delete el.second;
            }
            // limpa o buffer para receber mais mensagens na outra thread
            bridge19Buf.clear();

            for (auto el : posBuf)
            {

                // Processa as mensagens armazenadas no buffer de posicao
                PDUCache *pduCache = positionToPDU(el.first, *el.second);
                if (pduCache != nullptr)
                {
                    pduQueue.push(pduCache);
                }
                else
                {
                    ASA_LOG(error) << "[ERROR] Error processing posBuf : " << el.first << ".";
                }
                delete el.second;
            }

            // Limpa o buffer para receber mais mensagens na outra thread
            posBuf.clear();
        }

        // Se tiver algo na fila de PDUs, retorna o primeiro elemento
        size = getNextPDU(packet);
        if (size > 0)
        {
            return size;
        }

        return 0;
    }

    mixr::interop::Nib *AsaFGNetIO::createNewOutputNib(mixr::models::Player *const player)
    {
        mixr::interop::Nib *n = BaseClass::createNewOutputNib(player);
        return n;
    }

    std::string AsaFGNetIO::uniqueFGName(mixr::models::Player *player)
    {

        std::string name(player->getName()->getString());
        name = name.substr(0, MAX_CALLSIGN_LEN);
        std::string id = std::to_string(player->getID());

        name = name.substr(0, MAX_CALLSIGN_LEN - id.length() - 2);
        name.append("-");
        name.append(id);
        return name;
    }

    bool AsaFGNetIO::setSlotFgmsIP(mixr::base::String *const msg)
    {
        bool ok{};

        if (msg != nullptr)
        {
            ip = std::string(msg->getString());
            ok = true;
        }

        if (!ok)
            ASA_LOG(error) << "FGNetIO::setSlotFgmsIP(): invalid input.";

        return ok;
    }

    bool AsaFGNetIO::setSlotFgmsPort(mixr::base::Number *const msg)
    {
        bool ok{};

        if (msg != nullptr)
        {
            bool ok{};

            if (msg != nullptr)
            {
                port = msg->getInt();
                ok = true;
            }

            if (!ok)
                ASA_LOG(error) << "AsaStation::setSlotFgmsPort(): invalid input.";

            if (port <= 0 || port > 65535)
                ASA_LOG(error) << "AsaStation::setSlotFgmsPort(): invalid port: " << port;

            if (port <= 1024)
                ASA_LOG(warning) << "AsaStation::setSlotFgmsPort(): using an registered port: " << port;

            return ok;
        }

        if (!ok)
            ASA_LOG(error) << "FGNetIO::setSlotFgmsIP(): invalid input.";

        return ok;
    }

    bool AsaFGNetIO::setSlotMaxTimeDR(mixr::base::Number *const msg)
    {

        bool ok{};

        if (msg != nullptr)
        {
            bool ok{};

            if (msg != nullptr)
            {
                maxTimeDr = msg->getDouble();
                ok = true;
            }

            if (!ok)
                ASA_LOG(error) << "AsaStation::setSlotMaxTimeDR(): invalid input.";

            if (maxTimeDr <= 0)
            {
                ASA_LOG(error) << "AsaStation::setSlotMaxTimeDR(): invalid max time DR value: " << maxTimeDr << ". Using default: 1/25.";
                maxTimeDr = 1. / 25.;
            }

            return ok;
        }

        if (!ok)
            ASA_LOG(error) << "AsaFGNetIO::setSlotFgmsIP(): invalid input.";

        return ok;
    }

    bool AsaFGNetIO::setSlotBlueIffCode(mixr::base::String *const msg)
    {
        bool ok{};

        if (msg != nullptr)
        {
            blueIffCode = std::string(msg->getString());
            ok = true;
        }

        if (!ok)
            ASA_LOG(error) << "AsaFGNetIO::setSlotBlueIffCode(): invalid input.";

        return ok;
    }

    bool AsaFGNetIO::setSlotRedIffCode(mixr::base::String *const msg)
    {
        bool ok{};

        if (msg != nullptr)
        {
            redIffCode = std::string(msg->getString());
            ok = true;
        }

        if (!ok)
            ASA_LOG(error) << "AsaFGNetIO::setSlotRedIffCode(): invalid input.";

        return ok;
    }
}
