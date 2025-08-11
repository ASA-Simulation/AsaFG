#ifndef __ASA_FG_TRANSLATOR_HPP__
#define __ASA_FG_TRANSLATOR_HPP__

#include "asa/fg/Processmsg.hpp"

namespace asa::fg
{

    class Translator
    {
    public:
        Translator();
        ~Translator();
        bool unserialize(MsgBuf& msgBuf, FGExternalMotionData* motionData);
        int serialize(MsgBuf *msgBuf, FGExternalMotionData *motionData, std::string model, std::string callSign);

    private:
    };

}

#endif /* __ASA_FG_TRANSLATOR_HPP__  */