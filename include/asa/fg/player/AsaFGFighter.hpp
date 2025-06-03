#ifndef __asa_models_AsaFGFighter_H__
#define __asa_models_AsaFGFighter_H__

#include "asa/models/player/asa/air/AsaFighter.hpp"
#include "asa/fg/Mpmessages.hpp"
#include "asa/fg/player/AsaFGMissile.hpp"
#include "asa/models/system/AsaStoresMgr.hpp"
#include <unordered_map>

#include "mixr/base/osg/Vec2d"
#include "mixr/base/osg/Vec3f"
#include "mixr/models/Track.hpp"

namespace asa::fg
{

    class AsaFGFighter : public asa::models::AsaFighter
    {
        DECLARE_SUBCLASS(AsaFGFighter, asa::models::AsaFighter)
    public:
        AsaFGFighter();
        virtual double getMach() const override;
    };
}

#endif