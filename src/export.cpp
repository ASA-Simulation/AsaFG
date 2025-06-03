// asa::extensions
#include "asa/extensions/AsaExtension.hpp"
#include "asa/fg/AsaFGNetIO.hpp"
#include "asa/fg/player/AsaFGMissile.hpp"
#include "asa/fg/player/AsaFGFighter.hpp"
// asa::models
#include "asa/fg/Config.hpp"

namespace asa::fg
{

    ASA_EXTENSION_BEGIN(AsaFG, ASA_FG_VERSION_MAJOR, ASA_FG_VERSION_MINOR, ASA_FG_VERSION_PATCH)
    ASA_EXTENSION_REGISTER(AsaFGNetIO)
    ASA_EXTENSION_REGISTER(AsaFGMissile)
    ASA_EXTENSION_REGISTER(AsaFGFighter)
    ASA_EXTENSION_END()

}
