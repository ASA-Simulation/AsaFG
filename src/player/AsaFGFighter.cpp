#include "asa/fg/player/AsaFGFighter.hpp"
#include "asa/core/base/Log.hpp"
#include "asa/models/environment/AsaAtmosphere.hpp"
#include "asa/models/worldmodel/AsaWorldModel.hpp"
#include "asa/extensions/factory.hpp"
#include "asa/models/system/AsaStoresMgr.hpp"
#include "asa/models/system/AsaOnboardComputer.hpp"
#include "asa/models/system/trackmanager/AsaAirTrkMgr.hpp"
#include "asa/models-r/agent/pilot/AsaPilotState.hpp"
#include "asa/extensions/AsaStation.hpp"
#include "asa/core/base/Utils.hpp"
#include "asa/fg/Utils.hpp"
#include "asa/models/environment/AsaAtmosphere.hpp"
#include "asa/models/worldmodel/AsaWorldModel.hpp"

#include "mixr/models/dynamics/AerodynamicsModel.hpp"
#include "mixr/models/player/weapon/Missile.hpp"
#include "mixr/models/Track.hpp"
#include "mixr/base/osg/Matrixd"
#include "mixr/base/util/nav_utils.hpp"
#include <sstream>
#include <iostream>
#include <vector>
#include <cmath>

namespace asa::fg
{

    // clang-format off
    IMPLEMENT_SUBCLASS(AsaFGFighter, "AsaFGFighter")
    EMPTY_COPYDATA(AsaFGFighter)
    EMPTY_SLOTTABLE(AsaFGFighter)
    EMPTY_DELETEDATA(AsaFGFighter)
    // clang-format on

    AsaFGFighter::AsaFGFighter()
    {
        STANDARD_CONSTRUCTOR()
    }

    double AsaFGFighter::getMach() const
    {
        auto awm{dynamic_cast<const asa::models::AsaWorldModel *>(getWorldModel())};
        if (awm != nullptr)
        {
            auto atm{dynamic_cast<const asa::models::AsaAtmosphere *>(awm->getAtmosphere())};
            if (atm != nullptr)
            {
                double soundSpd{atm->getSoundSpeedISA(getAltitudeM())};
                auto velVecNED = getVelocity();
                auto vp = std::sqrt(velVecNED[0] * velVecNED[0] + velVecNED[1] * velVecNED[1] + velVecNED[2] * velVecNED[2]); // Total
                double mach{vp / soundSpd};
                return mach;
            }
            else
                return BaseClass::getMach();
        }
        else
            return BaseClass::getMach();
    }

}