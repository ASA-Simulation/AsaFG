#ifndef __XDR_UTILS__HXX__
#define __XDR_UTILS__HXX__

// Do not remove the include <boost/core/enable_if.hpp>. Need to avoid redefinition of structures by <simgear/props/props.hxx>
#include <boost/core/enable_if.hpp>
#include <simgear/props/props.hxx>

#include "asa/fg/Mpmessages.hpp"

namespace asa::fg
{

    std::string strProp(simgear::props::Type t);
    void sprintProperty(std::stringstream &ss, FGPropertyData *p);
    asa::fg::FGPropertyData* findPropertyMotionInfo (FGExternalMotionData *motionInfo, unsigned int id);
    asa::fg::FGPropertyData* createProperty (int id);
    void setSmoke(FGExternalMotionData *motionInfo, int value);
    void setSmokeGreen(FGExternalMotionData *motionInfo, int value);
    void setSmokeRed(FGExternalMotionData *motionInfo, int value);
    void setSmokeBlue(FGExternalMotionData *motionInfo, int value);
    void setSmokeWhite(FGExternalMotionData *motionInfo, int value);
    void setLivery(FGExternalMotionData *motionInfo, std::string value);
    void setSpeed(FGExternalMotionData *motionInfo, float value);
    void setMach(FGExternalMotionData *motionInfo, float value);
    void setStr4Iff(FGExternalMotionData *motionInfo, std::string value);
    void setStr5Pylons(FGExternalMotionData *motionInfo, std::string value);
    void setStr6Rwr(FGExternalMotionData *motionInfo, std::string value);
    void setStr7DataLink(FGExternalMotionData *motionInfo, std::string value);
    void setStr8DataLink(FGExternalMotionData *motionInfo, std::string value);
    void setGearUp(FGExternalMotionData *motionInfo, int value);
    void setInFligthNotification(FGExternalMotionData *motionInfo, std::string value);
    void setHitNotification(FGExternalMotionData *motionInfo, std::string value);
    double getElement(const std::vector<double>& matrix, int rows, int cols, int i, int j);
    void setElement(std::vector<double>& matrix, int rows, int cols, int i, int j, double value);
    std::vector<double> multiplyMatrices(const std::vector<double>& A, const std::vector<double>& B, int rows, int cols);
    std::vector<double> multiplyMatrixVector(const std::vector<double>& A, const std::vector<double>& v, int rows, int cols);



    
}

#endif /* __XDR_UTILS__HXX__ */