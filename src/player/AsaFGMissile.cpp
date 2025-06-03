#include "asa/fg/player/AsaFGMissile.hpp"

// mixr
#include "mixr/base/Identifier.hpp"
#include "mixr/base/List.hpp"
#include "mixr/models/WorldModel.hpp"
#include "mixr/models/player/Player.hpp"
#include "mixr/base/util/nav_utils.hpp"

// asa
#include "asa/models/player/mixr_inheritance/air/AsaAircraft.hpp"
#include "asa/models/miscellaneous/AsaEventsTokens.hpp"
#include "asa/core/base/Log.hpp"

namespace asa::fg
{
    // clang-format off
IMPLEMENT_ABSTRACT_SUBCLASS(AsaFGAbstractMissile, "AsaFGAbstractMissile")
EMPTY_SLOTTABLE(AsaFGAbstractMissile)
EMPTY_DELETEDATA(AsaFGAbstractMissile)
    // clang-format on
    AsaFGAbstractMissile::AsaFGAbstractMissile()
    {
        STANDARD_CONSTRUCTOR()
    }
    //------------------------------------------------------------------------------
    // clang-format on

    void AsaFGAbstractMissile::copyData(const AsaFGAbstractMissile &org, const bool)
    {
        BaseClass::copyData(org);

        seekerOnDist = org.seekerOnDist;
        seekerGimbal = org.seekerGimbal;
    }
    //------------------------------------------------------------------------------

    void AsaFGAbstractMissile::dynamics(const double dt)
    {
        BaseClass::dynamics(dt);

        // se for um missil radar-ativo (F3), avaliar se deve ligar o seeker
        if (getAsaWeaponSensor() == asa::models::AsaWeaponData::AsaWeaponSensor::ACTIVE_RADAR && !seekerOn)
        {
            auto plist{getWorldModel()->getPlayers()};
            if (plist != nullptr)
            {
                auto item{plist->getFirstItem()};
                while (item != nullptr)
                {
                    auto pair{static_cast<mixr::base::Pair *>(item->getValue())};
                    auto acft{dynamic_cast<asa::models::AsaAircraft *>(pair->object())};

                    if (acft != nullptr && acft->isLocalPlayer() && acft->isActive() && !acft->isDead())
                    {
                        // vetor diferenca de posicoes (target - missile)
                        mixr::base::Vec3d ur{acft->getGeocPosition() - getGeocPosition()}; // ECEF
                        double range{ur.normalize()};

                        // vetor center line do missil
                        mixr::base::Vec3d uv(getVelocityBody().x(), 0, 0);
                        uv = uv * getRotMat();   // from Body to NED
                        uv = uv * getWorldMat(); // from NED to ECEF
                        uv.normalize();

                        // off-boresight angle
                        double off{acos(ur * uv) * mixr::base::angle::R2DCC};

                        // dentro do gimbal e a uma distancia menor que a de ativacao
                        if (off <= seekerGimbal && (range > 500.0 && range <= seekerOnDist))
                        {
                            seekerOn = true;
                            break;
                        }
                    }

                    item = item->getNext();
                }

                plist->unref();
                plist = nullptr;
            }
        }

        // se for um missil F3 e estiver ativo, enviar alarme para as aeronaves que estiverem no FOV
        if (getAsaWeaponSensor() == asa::models::AsaWeaponData::AsaWeaponSensor::ACTIVE_RADAR && seekerOn)
        {
            auto plist{getWorldModel()->getPlayers()};
            if (plist != nullptr)
            {
                auto item{plist->getFirstItem()};
                while (item != nullptr)
                {
                    auto pair{static_cast<mixr::base::Pair *>(item->getValue())};
                    auto acft{dynamic_cast<asa::models::AsaAircraft *>(pair->object())};

                    if (acft != nullptr && acft->isLocalPlayer() && acft->isActive() && !acft->isDead())
                    {
                        // vetor diferenca de posicoes (target - missile)
                        mixr::base::Vec3d ur{acft->getGeocPosition() - getGeocPosition()}; // ECEF
                        double range{ur.normalize()};

                        // vetor center line do missil
                        mixr::base::Vec3d uv(getVelocityBody().x(), 0, 0);
                        uv = uv * getRotMat();   // from Body to NED
                        uv = uv * getWorldMat(); // from NED to ECEF
                        uv.normalize();

                        // off-boresight angle
                        double off{acos(ur * uv) * mixr::base::angle::R2DCC};

                        if (off <= seekerGimbal / 1.0 && range <= 1.5 * seekerOnDist)
                        {
                            double brg{}, dist{};
                            mixr::base::nav::gll2bd(acft->getLatitude(), acft->getLongitude(),
                                                    getLatitude(), getLongitude(),
                                                    &brg, &dist);

                            mixr::base::Number taz{brg};
                            acft->event(asa::models::ASA_OUTER_MISSILE_WARNING, &taz);
                        }
                    }

                    item = item->getNext();
                }

                plist->unref();
                plist = nullptr;
            }
        }
    }

    //------------------------------------------------------------------------------
    // Class: AsaFGMissile
    // Factory name: AsaFGMissile
    //------------------------------------------------------------------------------
    IMPLEMENT_SUBCLASS(AsaFGMissile, "AsaFGMissile")
    EMPTY_SLOTTABLE(AsaFGMissile)
    EMPTY_COPYDATA(AsaFGMissile)
    EMPTY_DELETEDATA(AsaFGMissile)

    const char *AsaFGMissile::getDescription() const { return "AsaFGMissile"; }
    const char *AsaFGMissile::getNickname() const { return "AIM-120C"; } // esse valor está sendo usado no Tacview

    AsaFGMissile::AsaFGMissile()
    {
        STANDARD_CONSTRUCTOR()

        static mixr::base::String generic("AIM-120C"); // representacao no TacView
        setType(&generic);

        setMaxBurstRng(150.0);
        setLethalRange(25.0);

        seekerGimbal = 30.0;
        seekerOnDist = 4.8 * mixr::base::distance::NM2M;

        setDataLogInterval(0.1);
    }
    //------------------------------------------------------------------------------
    // clang-format on

    asa::models::AsaWeaponData::AsaWeaponType AsaFGMissile::getAsaWeaponType()
    {
        return asa::models::AsaWeaponData::AsaWeaponType::MISSILE;
    }
    //------------------------------------------------------------------------------

    asa::models::AsaWeaponData::AsaWeaponSubtype AsaFGMissile::getAsaWeaponSubtype()
    {
        return asa::models::AsaWeaponData::AsaWeaponSubtype::AIR_AIR_MSL;
    }
    //------------------------------------------------------------------------------

    asa::models::AsaWeaponData::AsaWeaponSensor AsaFGMissile::getAsaWeaponSensor()
    {
        return asa::models::AsaWeaponData::AsaWeaponSensor::ACTIVE_RADAR;
    }
    //------------------------------------------------------------------------------

    double AsaFGMissile::getGrossWeight() const
    {
        // Mesmo usado no Fox3
        return (120 * mixr::base::mass::KG2PM);
    }
}