#ifndef __ASAFG_PROCESSMSG_HPP__
#define __ASAFG_PROCESSMSG_HPP__

#include <iostream>
#include <algorithm>
#include <cstring>
#include <errno.h>
#include <deque>
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <arpa/inet.h>

#include <boost/core/enable_if.hpp>
#include <simgear/math/sg_random.h>
#include <simgear/misc/stdint.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/compiler.h>
#include <simgear/io/raw_socket.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include "asa/fg/Mpmessages.hpp"

/* *********************************************************************************
 * Defines
 * *********************************************************************************/

#define MAX_PACKET_SIZE 1200
#define MAX_TEXT_SIZE 768 // Increased for 2017.3 to allow for long Emesary messages.
#define REPLY_PORT_DFT 1111 // Default Reply port
namespace asa::fg
{

    /* *********************************************************************************
     * Structs, Classes and Unions
     * *********************************************************************************/

    /**
     * The buffer that holds a multi-player message, suitably aligned.
     */
    union MsgBuf
    {
        MsgBuf()
        {
            memset(&Msg, 0, sizeof(Msg));
        }

        T_MsgHdr *msgHdr()
        {
            return &Header;
        }

        const T_MsgHdr *msgHdr() const
        {
            return reinterpret_cast<const T_MsgHdr *>(&Header);
        }

        T_PositionMsg *posMsg()
        {
            return reinterpret_cast<T_PositionMsg *>(Msg + sizeof(T_MsgHdr));
        }

        const T_PositionMsg *posMsg() const
        {
            return reinterpret_cast<const T_PositionMsg *>(Msg + sizeof(T_MsgHdr));
        }

        xdr_data_t *properties()
        {
            return reinterpret_cast<xdr_data_t *>(Msg + sizeof(T_MsgHdr) + sizeof(T_PositionMsg));
        }

        const xdr_data_t *properties() const
        {
            return reinterpret_cast<const xdr_data_t *>(Msg + sizeof(T_MsgHdr) + sizeof(T_PositionMsg));
        }
        /**
         * The end of the properties buffer.
         */
        xdr_data_t *propsEnd()
        {
            return reinterpret_cast<xdr_data_t *>(Msg + MAX_PACKET_SIZE);
        };

        const xdr_data_t *propsEnd() const
        {
            return reinterpret_cast<const xdr_data_t *>(Msg + MAX_PACKET_SIZE);
        };
        /**
         * The end of properties actually in the buffer. This assumes that
         * the message header is valid.
         */
        xdr_data_t *propsRecvdEnd()
        {
            return reinterpret_cast<xdr_data_t *>(Msg + Header.MsgLen);
        }

        const xdr_data_t *propsRecvdEnd() const
        {
            return reinterpret_cast<const xdr_data_t *>(Msg + Header.MsgLen);
        }

        xdr_data2_t double_val;
        char Msg[MAX_PACKET_SIZE];
        T_MsgHdr Header;
    };

    enum TransmissionType
    {
        TT_ASIS = 0, // transmit as defined in the property. This is the default
        TT_BOOL = simgear::props::BOOL,
        TT_INT = simgear::props::INT,
        TT_FLOAT = simgear::props::FLOAT,
        TT_STRING = simgear::props::STRING,
        TT_SHORTINT = 0x100,
        TT_SHORT_FLOAT_NORM = 0x101, // -1 .. 1 encoded into a short int (16 bit)
        TT_SHORT_FLOAT_1 = 0x102,    // range -3276.7 .. 3276.7  float encoded into a short int (16 bit)
        TT_SHORT_FLOAT_2 = 0x103,    // range -327.67 .. 327.67  float encoded into a short int (16 bit)
        TT_SHORT_FLOAT_3 = 0x104,    // range -32.767 .. 32.767  float encoded into a short int (16 bit)
        TT_SHORT_FLOAT_4 = 0x105,    // range -3.2767 .. 3.2767  float encoded into a short int (16 bit)
        TT_BOOLARRAY,
        TT_CHAR,
        TT_NOSEND, // Do not send this property - probably the receive element for a custom encoded property
    };

    struct IdPropertyList
    {
        unsigned id;
        const char *name;
        simgear::props::Type type;
        TransmissionType TransmitAs;
        int version;
        xdr_data_t *(*encode_for_transmit)(const IdPropertyList *propDef, const xdr_data_t *, FGPropertyData *);
        xdr_data_t *(*decode_received)(const IdPropertyList *propDef, const xdr_data_t *, FGPropertyData *);
    };

    typedef std::map<unsigned int, const struct IdPropertyList *> PropertyDefinitionMap;

    struct ComparePropertyId
    {
        bool operator()(const IdPropertyList &lhs,
                        const IdPropertyList &rhs)
        {
            return lhs.id < rhs.id;
        }
        bool operator()(const IdPropertyList &lhs,
                        unsigned id)
        {
            return lhs.id < id;
        }
        bool operator()(unsigned id,
                        const IdPropertyList &rhs)
        {
            return id < rhs.id;
        }
    };

    struct BoolArrayBuffer
    {
        int propertyId;
        int boolValue;
    };

    typedef std::map<unsigned int, const struct IdPropertyList *> PropertyDefinitionMap;

    /* *********************************************************************************
     * Constansts and global vars
     * *********************************************************************************/

    const int V1_1_PROP_ID = 1;
    const int V1_1_2_PROP_ID = 2;
    const int V2_PROP_ID_PROTOCOL = 0x10001;

    const int V2_PAD_MAGIC = 0x1face002;

    const int BOOLARRAY_BLOCKSIZE = 40;

    const int BOOLARRAY_BASE_1 = 11000;
    const int BOOLARRAY_BASE_2 = BOOLARRAY_BASE_1 + BOOLARRAY_BLOCKSIZE;
    const int BOOLARRAY_BASE_3 = BOOLARRAY_BASE_2 + BOOLARRAY_BLOCKSIZE;
    // define range of bool values for receive.
    const int BOOLARRAY_START_ID = BOOLARRAY_BASE_1;
    const int BOOLARRAY_END_ID = BOOLARRAY_BASE_3;
    // Transmission uses a buffer to build the value for each array block.
    const int MAX_BOOL_BUFFERS = 3;

    // starting with 2018.1 we will now append new properties for each version at the end of the list - as this provides much
    // better backwards compatibility.
    const int V2018_1_BASE = 11990;
    const int EMESARYBRIDGETYPE_BASE = 12200;
    const int EMESARYBRIDGE_BASE = 12000;
    const int V2018_3_BASE = 13000;
    const int FALLBACK_MODEL_ID = 13000;
    const int V2019_3_BASE = 13001;

    const int MAX_PARTITIONS = 2;

    static xdr_data_t *encode_launchbar_state_for_transmission(const IdPropertyList *propDef, const xdr_data_t *_xdr, FGPropertyData *p)
    {
        xdr_data_t *xdr = (xdr_data_t *)_xdr;

        if (propDef->TransmitAs == TT_NOSEND)
            return xdr;

        int v = -1;
        if (p && p->string_value)
        {
            if (!strcmp("Engaged", p->string_value))
                v = 0;
            else if (!strcmp("Launching", p->string_value))
                v = 1;
            else if (!strcmp("Completed", p->string_value))
                v = 2;
            else if (!strcmp("Disengaged", p->string_value))
                v = 3;
            else
                return (xdr_data_t *)xdr;

            *xdr++ = XDR_encode_shortints32(120, v);
        }
        return xdr;
    }

    static xdr_data_t *decode_received_launchbar_state(const IdPropertyList *propDef, const xdr_data_t *_xdr, FGPropertyData *p)
    {
        xdr_data_t *xdr = (xdr_data_t *)_xdr;

        int v1, v2;
        XDR_decode_shortints32(*xdr, v1, v2);
        xdr++;
        const char *stringvalue = "";
        switch (v2)
        {
        case 0:
            stringvalue = "Engaged";
            break;
        case 1:
            stringvalue = "Launching";
            break;
        case 2:
            stringvalue = "Completed";
            break;
        case 3:
            stringvalue = "Disengaged";
            break;
        }

        p->id = 108; // this is for the string property for gear/launchbar/state
        if (p->string_value && p->type == simgear::props::STRING)
            delete[] p->string_value;
        p->string_value = new char[strlen(stringvalue) + 1];
        strcpy(p->string_value, stringvalue);
        p->type = simgear::props::STRING;
        return xdr;
    }

    // A static map of protocol property id values to property paths,
    // This should be extendable dynamically for every specific aircraft ...
    // For now only that static list
    static const IdPropertyList sIdPropertyList[] = {
        {10, "sim/multiplay/protocol-version", simgear::props::INT, TT_SHORTINT, V2_PROP_ID_PROTOCOL, NULL, NULL},
        {100, "surface-positions/left-aileron-pos-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {101, "surface-positions/right-aileron-pos-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {102, "surface-positions/elevator-pos-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {103, "surface-positions/rudder-pos-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {104, "surface-positions/flap-pos-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {105, "surface-positions/speedbrake-pos-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {106, "gear/tailhook/position-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {107, "gear/launchbar/position-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        //
        {108, "gear/launchbar/state", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, encode_launchbar_state_for_transmission, NULL},
        {109, "gear/launchbar/holdback-position-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {110, "canopy/position-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {111, "surface-positions/wing-pos-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {112, "surface-positions/wing-fold-pos-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},

        // to enable decoding this is the transient ID record that is in the packet. This is not sent directly - instead it is the result
        // of the conversion of property 108.
        {120, "gear/launchbar/state-value", simgear::props::INT, TT_NOSEND, V1_1_2_PROP_ID, NULL, decode_received_launchbar_state},

        {200, "gear/gear[0]/compression-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {201, "gear/gear[0]/position-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {210, "gear/gear[1]/compression-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {211, "gear/gear[1]/position-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {220, "gear/gear[2]/compression-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {221, "gear/gear[2]/position-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {230, "gear/gear[3]/compression-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {231, "gear/gear[3]/position-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {240, "gear/gear[4]/compression-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},
        {241, "gear/gear[4]/position-norm", simgear::props::FLOAT, TT_SHORT_FLOAT_NORM, V1_1_PROP_ID, NULL, NULL},

        {300, "engines/engine[0]/n1", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {301, "engines/engine[0]/n2", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {302, "engines/engine[0]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {310, "engines/engine[1]/n1", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {311, "engines/engine[1]/n2", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {312, "engines/engine[1]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {320, "engines/engine[2]/n1", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {321, "engines/engine[2]/n2", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {322, "engines/engine[2]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {330, "engines/engine[3]/n1", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {331, "engines/engine[3]/n2", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {332, "engines/engine[3]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {340, "engines/engine[4]/n1", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {341, "engines/engine[4]/n2", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {342, "engines/engine[4]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {350, "engines/engine[5]/n1", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {351, "engines/engine[5]/n2", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {352, "engines/engine[5]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {360, "engines/engine[6]/n1", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {361, "engines/engine[6]/n2", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {362, "engines/engine[6]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {370, "engines/engine[7]/n1", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {371, "engines/engine[7]/n2", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {372, "engines/engine[7]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {380, "engines/engine[8]/n1", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {381, "engines/engine[8]/n2", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {382, "engines/engine[8]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {390, "engines/engine[9]/n1", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {391, "engines/engine[9]/n2", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {392, "engines/engine[9]/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},

        {800, "rotors/main/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {801, "rotors/tail/rpm", simgear::props::FLOAT, TT_SHORT_FLOAT_1, V1_1_PROP_ID, NULL, NULL},
        {810, "rotors/main/blade[0]/position-deg", simgear::props::FLOAT, TT_SHORT_FLOAT_3, V1_1_PROP_ID, NULL, NULL},
        {811, "rotors/main/blade[1]/position-deg", simgear::props::FLOAT, TT_SHORT_FLOAT_3, V1_1_PROP_ID, NULL, NULL},
        {812, "rotors/main/blade[2]/position-deg", simgear::props::FLOAT, TT_SHORT_FLOAT_3, V1_1_PROP_ID, NULL, NULL},
        {813, "rotors/main/blade[3]/position-deg", simgear::props::FLOAT, TT_SHORT_FLOAT_3, V1_1_PROP_ID, NULL, NULL},
        {820, "rotors/main/blade[0]/flap-deg", simgear::props::FLOAT, TT_SHORT_FLOAT_3, V1_1_PROP_ID, NULL, NULL},
        {821, "rotors/main/blade[1]/flap-deg", simgear::props::FLOAT, TT_SHORT_FLOAT_3, V1_1_PROP_ID, NULL, NULL},
        {822, "rotors/main/blade[2]/flap-deg", simgear::props::FLOAT, TT_SHORT_FLOAT_3, V1_1_PROP_ID, NULL, NULL},
        {823, "rotors/main/blade[3]/flap-deg", simgear::props::FLOAT, TT_SHORT_FLOAT_3, V1_1_PROP_ID, NULL, NULL},
        {830, "rotors/tail/blade[0]/position-deg", simgear::props::FLOAT, TT_SHORT_FLOAT_3, V1_1_PROP_ID, NULL, NULL},
        {831, "rotors/tail/blade[1]/position-deg", simgear::props::FLOAT, TT_SHORT_FLOAT_3, V1_1_PROP_ID, NULL, NULL},

        {900, "sim/hitches/aerotow/tow/length", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {901, "sim/hitches/aerotow/tow/elastic-constant", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {902, "sim/hitches/aerotow/tow/weight-per-m-kg-m", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {903, "sim/hitches/aerotow/tow/dist", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {904, "sim/hitches/aerotow/tow/connected-to-property-node", simgear::props::BOOL, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {905, "sim/hitches/aerotow/tow/connected-to-ai-or-mp-callsign", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {906, "sim/hitches/aerotow/tow/brake-force", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {907, "sim/hitches/aerotow/tow/end-force-x", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {908, "sim/hitches/aerotow/tow/end-force-y", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {909, "sim/hitches/aerotow/tow/end-force-z", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {930, "sim/hitches/aerotow/is-slave", simgear::props::BOOL, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {931, "sim/hitches/aerotow/speed-in-tow-direction", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {932, "sim/hitches/aerotow/open", simgear::props::BOOL, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {933, "sim/hitches/aerotow/local-pos-x", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {934, "sim/hitches/aerotow/local-pos-y", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {935, "sim/hitches/aerotow/local-pos-z", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},

        {1001, "controls/flight/slats", simgear::props::FLOAT, TT_SHORT_FLOAT_4, V1_1_PROP_ID, NULL, NULL},
        {1002, "controls/flight/speedbrake", simgear::props::FLOAT, TT_SHORT_FLOAT_4, V1_1_PROP_ID, NULL, NULL},
        {1003, "controls/flight/spoilers", simgear::props::FLOAT, TT_SHORT_FLOAT_4, V1_1_PROP_ID, NULL, NULL},
        {1004, "controls/gear/gear-down", simgear::props::FLOAT, TT_SHORT_FLOAT_4, V1_1_PROP_ID, NULL, NULL},
        {1005, "controls/lighting/nav-lights", simgear::props::FLOAT, TT_SHORT_FLOAT_3, V1_1_PROP_ID, NULL, NULL},
        {1006, "controls/armament/station[0]/jettison-all", simgear::props::BOOL, TT_SHORTINT, V1_1_PROP_ID, NULL, NULL},

        {1100, "sim/model/variant", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {1101, "sim/model/livery/file", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},

        {1200, "environment/wildfire/data", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {1201, "environment/contrail", simgear::props::INT, TT_SHORTINT, V1_1_PROP_ID, NULL, NULL},

        {1300, "tanker", simgear::props::INT, TT_SHORTINT, V1_1_PROP_ID, NULL, NULL},

        {1400, "scenery/events", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},

        {1500, "instrumentation/transponder/transmitted-id", simgear::props::INT, TT_SHORTINT, V1_1_PROP_ID, NULL, NULL},
        {1501, "instrumentation/transponder/altitude", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {1502, "instrumentation/transponder/ident", simgear::props::BOOL, TT_SHORTINT, V1_1_PROP_ID, NULL, NULL},
        {1503, "instrumentation/transponder/inputs/mode", simgear::props::INT, TT_SHORTINT, V1_1_PROP_ID, NULL, NULL},
        {1504, "instrumentation/transponder/ground-bit", simgear::props::BOOL, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {1505, "instrumentation/transponder/airspeed-kt", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},

        {10001, "sim/multiplay/transmission-freq-hz", simgear::props::STRING, TT_NOSEND, V1_1_2_PROP_ID, NULL, NULL},
        {10002, "sim/multiplay/chat", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},

        {10100, "sim/multiplay/generic/string[0]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10101, "sim/multiplay/generic/string[1]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10102, "sim/multiplay/generic/string[2]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10103, "sim/multiplay/generic/string[3]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10104, "sim/multiplay/generic/string[4]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10105, "sim/multiplay/generic/string[5]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10106, "sim/multiplay/generic/string[6]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10107, "sim/multiplay/generic/string[7]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10108, "sim/multiplay/generic/string[8]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10109, "sim/multiplay/generic/string[9]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10110, "sim/multiplay/generic/string[10]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10111, "sim/multiplay/generic/string[11]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10112, "sim/multiplay/generic/string[12]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10113, "sim/multiplay/generic/string[13]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10114, "sim/multiplay/generic/string[14]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10115, "sim/multiplay/generic/string[15]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10116, "sim/multiplay/generic/string[16]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10117, "sim/multiplay/generic/string[17]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10118, "sim/multiplay/generic/string[18]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {10119, "sim/multiplay/generic/string[19]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},

        {10200, "sim/multiplay/generic/float[0]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10201, "sim/multiplay/generic/float[1]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10202, "sim/multiplay/generic/float[2]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10203, "sim/multiplay/generic/float[3]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10204, "sim/multiplay/generic/float[4]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10205, "sim/multiplay/generic/float[5]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10206, "sim/multiplay/generic/float[6]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10207, "sim/multiplay/generic/float[7]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10208, "sim/multiplay/generic/float[8]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10209, "sim/multiplay/generic/float[9]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10210, "sim/multiplay/generic/float[10]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10211, "sim/multiplay/generic/float[11]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10212, "sim/multiplay/generic/float[12]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10213, "sim/multiplay/generic/float[13]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10214, "sim/multiplay/generic/float[14]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10215, "sim/multiplay/generic/float[15]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10216, "sim/multiplay/generic/float[16]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10217, "sim/multiplay/generic/float[17]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10218, "sim/multiplay/generic/float[18]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10219, "sim/multiplay/generic/float[19]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},

        {10220, "sim/multiplay/generic/float[20]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10221, "sim/multiplay/generic/float[21]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10222, "sim/multiplay/generic/float[22]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10223, "sim/multiplay/generic/float[23]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10224, "sim/multiplay/generic/float[24]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10225, "sim/multiplay/generic/float[25]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10226, "sim/multiplay/generic/float[26]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10227, "sim/multiplay/generic/float[27]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10228, "sim/multiplay/generic/float[28]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10229, "sim/multiplay/generic/float[29]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10230, "sim/multiplay/generic/float[30]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10231, "sim/multiplay/generic/float[31]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10232, "sim/multiplay/generic/float[32]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10233, "sim/multiplay/generic/float[33]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10234, "sim/multiplay/generic/float[34]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10235, "sim/multiplay/generic/float[35]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10236, "sim/multiplay/generic/float[36]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10237, "sim/multiplay/generic/float[37]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10238, "sim/multiplay/generic/float[38]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10239, "sim/multiplay/generic/float[39]", simgear::props::FLOAT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},

        {10300, "sim/multiplay/generic/int[0]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10301, "sim/multiplay/generic/int[1]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10302, "sim/multiplay/generic/int[2]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10303, "sim/multiplay/generic/int[3]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10304, "sim/multiplay/generic/int[4]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10305, "sim/multiplay/generic/int[5]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10306, "sim/multiplay/generic/int[6]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10307, "sim/multiplay/generic/int[7]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10308, "sim/multiplay/generic/int[8]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10309, "sim/multiplay/generic/int[9]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10310, "sim/multiplay/generic/int[10]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10311, "sim/multiplay/generic/int[11]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10312, "sim/multiplay/generic/int[12]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10313, "sim/multiplay/generic/int[13]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10314, "sim/multiplay/generic/int[14]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10315, "sim/multiplay/generic/int[15]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10316, "sim/multiplay/generic/int[16]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10317, "sim/multiplay/generic/int[17]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10318, "sim/multiplay/generic/int[18]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},
        {10319, "sim/multiplay/generic/int[19]", simgear::props::INT, TT_ASIS, V1_1_PROP_ID, NULL, NULL},

        {10500, "sim/multiplay/generic/short[0]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10501, "sim/multiplay/generic/short[1]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10502, "sim/multiplay/generic/short[2]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10503, "sim/multiplay/generic/short[3]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10504, "sim/multiplay/generic/short[4]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10505, "sim/multiplay/generic/short[5]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10506, "sim/multiplay/generic/short[6]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10507, "sim/multiplay/generic/short[7]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10508, "sim/multiplay/generic/short[8]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10509, "sim/multiplay/generic/short[9]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10510, "sim/multiplay/generic/short[10]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10511, "sim/multiplay/generic/short[11]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10512, "sim/multiplay/generic/short[12]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10513, "sim/multiplay/generic/short[13]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10514, "sim/multiplay/generic/short[14]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10515, "sim/multiplay/generic/short[15]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10516, "sim/multiplay/generic/short[16]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10517, "sim/multiplay/generic/short[17]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10518, "sim/multiplay/generic/short[18]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10519, "sim/multiplay/generic/short[19]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10520, "sim/multiplay/generic/short[20]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10521, "sim/multiplay/generic/short[21]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10522, "sim/multiplay/generic/short[22]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10523, "sim/multiplay/generic/short[23]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10524, "sim/multiplay/generic/short[24]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10525, "sim/multiplay/generic/short[25]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10526, "sim/multiplay/generic/short[26]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10527, "sim/multiplay/generic/short[27]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10528, "sim/multiplay/generic/short[28]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10529, "sim/multiplay/generic/short[29]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10530, "sim/multiplay/generic/short[30]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10531, "sim/multiplay/generic/short[31]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10532, "sim/multiplay/generic/short[32]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10533, "sim/multiplay/generic/short[33]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10534, "sim/multiplay/generic/short[34]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10535, "sim/multiplay/generic/short[35]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10536, "sim/multiplay/generic/short[36]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10537, "sim/multiplay/generic/short[37]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10538, "sim/multiplay/generic/short[38]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10539, "sim/multiplay/generic/short[39]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10540, "sim/multiplay/generic/short[40]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10541, "sim/multiplay/generic/short[41]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10542, "sim/multiplay/generic/short[42]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10543, "sim/multiplay/generic/short[43]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10544, "sim/multiplay/generic/short[44]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10545, "sim/multiplay/generic/short[45]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10546, "sim/multiplay/generic/short[46]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10547, "sim/multiplay/generic/short[47]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10548, "sim/multiplay/generic/short[48]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10549, "sim/multiplay/generic/short[49]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10550, "sim/multiplay/generic/short[50]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10551, "sim/multiplay/generic/short[51]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10552, "sim/multiplay/generic/short[52]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10553, "sim/multiplay/generic/short[53]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10554, "sim/multiplay/generic/short[54]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10555, "sim/multiplay/generic/short[55]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10556, "sim/multiplay/generic/short[56]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10557, "sim/multiplay/generic/short[57]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10558, "sim/multiplay/generic/short[58]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10559, "sim/multiplay/generic/short[59]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10560, "sim/multiplay/generic/short[60]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10561, "sim/multiplay/generic/short[61]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10562, "sim/multiplay/generic/short[62]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10563, "sim/multiplay/generic/short[63]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10564, "sim/multiplay/generic/short[64]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10565, "sim/multiplay/generic/short[65]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10566, "sim/multiplay/generic/short[66]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10567, "sim/multiplay/generic/short[67]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10568, "sim/multiplay/generic/short[68]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10569, "sim/multiplay/generic/short[69]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10570, "sim/multiplay/generic/short[70]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10571, "sim/multiplay/generic/short[71]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10572, "sim/multiplay/generic/short[72]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10573, "sim/multiplay/generic/short[73]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10574, "sim/multiplay/generic/short[74]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10575, "sim/multiplay/generic/short[75]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10576, "sim/multiplay/generic/short[76]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10577, "sim/multiplay/generic/short[77]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10578, "sim/multiplay/generic/short[78]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {10579, "sim/multiplay/generic/short[79]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},

        {BOOLARRAY_BASE_1 + 0, "sim/multiplay/generic/bool[0]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 1, "sim/multiplay/generic/bool[1]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 2, "sim/multiplay/generic/bool[2]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 3, "sim/multiplay/generic/bool[3]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 4, "sim/multiplay/generic/bool[4]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 5, "sim/multiplay/generic/bool[5]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 6, "sim/multiplay/generic/bool[6]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 7, "sim/multiplay/generic/bool[7]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 8, "sim/multiplay/generic/bool[8]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 9, "sim/multiplay/generic/bool[9]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 10, "sim/multiplay/generic/bool[10]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 11, "sim/multiplay/generic/bool[11]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 12, "sim/multiplay/generic/bool[12]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 13, "sim/multiplay/generic/bool[13]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 14, "sim/multiplay/generic/bool[14]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 15, "sim/multiplay/generic/bool[15]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 16, "sim/multiplay/generic/bool[16]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 17, "sim/multiplay/generic/bool[17]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 18, "sim/multiplay/generic/bool[18]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 19, "sim/multiplay/generic/bool[19]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 20, "sim/multiplay/generic/bool[20]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 21, "sim/multiplay/generic/bool[21]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 22, "sim/multiplay/generic/bool[22]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 23, "sim/multiplay/generic/bool[23]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 24, "sim/multiplay/generic/bool[24]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 25, "sim/multiplay/generic/bool[25]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 26, "sim/multiplay/generic/bool[26]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 27, "sim/multiplay/generic/bool[27]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 28, "sim/multiplay/generic/bool[28]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 29, "sim/multiplay/generic/bool[29]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_1 + 30, "sim/multiplay/generic/bool[30]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},

        {BOOLARRAY_BASE_2 + 0, "sim/multiplay/generic/bool[31]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 1, "sim/multiplay/generic/bool[32]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 2, "sim/multiplay/generic/bool[33]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 3, "sim/multiplay/generic/bool[34]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 4, "sim/multiplay/generic/bool[35]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 5, "sim/multiplay/generic/bool[36]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 6, "sim/multiplay/generic/bool[37]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 7, "sim/multiplay/generic/bool[38]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 8, "sim/multiplay/generic/bool[39]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 9, "sim/multiplay/generic/bool[40]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 10, "sim/multiplay/generic/bool[41]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        // out of sequence between the block and the buffer becuase of a typo. repurpose the first as that way [72] will work
        // correctly on older versions.
        {BOOLARRAY_BASE_2 + 11, "sim/multiplay/generic/bool[91]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 12, "sim/multiplay/generic/bool[42]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 13, "sim/multiplay/generic/bool[43]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 14, "sim/multiplay/generic/bool[44]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 15, "sim/multiplay/generic/bool[45]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 16, "sim/multiplay/generic/bool[46]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 17, "sim/multiplay/generic/bool[47]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 18, "sim/multiplay/generic/bool[48]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 19, "sim/multiplay/generic/bool[49]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 20, "sim/multiplay/generic/bool[50]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 21, "sim/multiplay/generic/bool[51]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 22, "sim/multiplay/generic/bool[52]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 23, "sim/multiplay/generic/bool[53]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 24, "sim/multiplay/generic/bool[54]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 25, "sim/multiplay/generic/bool[55]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 26, "sim/multiplay/generic/bool[56]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 27, "sim/multiplay/generic/bool[57]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 28, "sim/multiplay/generic/bool[58]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 29, "sim/multiplay/generic/bool[59]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_2 + 30, "sim/multiplay/generic/bool[60]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},

        {BOOLARRAY_BASE_3 + 0, "sim/multiplay/generic/bool[61]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 1, "sim/multiplay/generic/bool[62]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 2, "sim/multiplay/generic/bool[63]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 3, "sim/multiplay/generic/bool[64]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 4, "sim/multiplay/generic/bool[65]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 5, "sim/multiplay/generic/bool[66]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 6, "sim/multiplay/generic/bool[67]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 7, "sim/multiplay/generic/bool[68]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 8, "sim/multiplay/generic/bool[69]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 9, "sim/multiplay/generic/bool[70]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 10, "sim/multiplay/generic/bool[71]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        // out of sequence between the block and the buffer becuase of a typo. repurpose the first as that way [72] will work
        // correctly on older versions.
        {BOOLARRAY_BASE_3 + 11, "sim/multiplay/generic/bool[92]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 12, "sim/multiplay/generic/bool[72]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 13, "sim/multiplay/generic/bool[73]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 14, "sim/multiplay/generic/bool[74]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 15, "sim/multiplay/generic/bool[75]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 16, "sim/multiplay/generic/bool[76]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 17, "sim/multiplay/generic/bool[77]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 18, "sim/multiplay/generic/bool[78]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 19, "sim/multiplay/generic/bool[79]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 20, "sim/multiplay/generic/bool[80]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 21, "sim/multiplay/generic/bool[81]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 22, "sim/multiplay/generic/bool[82]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 23, "sim/multiplay/generic/bool[83]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 24, "sim/multiplay/generic/bool[84]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 25, "sim/multiplay/generic/bool[85]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 26, "sim/multiplay/generic/bool[86]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 27, "sim/multiplay/generic/bool[87]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 28, "sim/multiplay/generic/bool[88]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 29, "sim/multiplay/generic/bool[89]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},
        {BOOLARRAY_BASE_3 + 30, "sim/multiplay/generic/bool[90]", simgear::props::BOOL, TT_BOOLARRAY, V1_1_2_PROP_ID, NULL, NULL},

        {V2018_1_BASE + 0, "sim/multiplay/mp-clock-mode", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        // Direct support for emesary bridge properties. This is mainly to ensure that these properties do not overlap with the string
        // properties; although the emesary bridge can use any string property.
        {EMESARYBRIDGE_BASE + 0, "sim/multiplay/emesary/bridge[0]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 1, "sim/multiplay/emesary/bridge[1]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 2, "sim/multiplay/emesary/bridge[2]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 3, "sim/multiplay/emesary/bridge[3]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 4, "sim/multiplay/emesary/bridge[4]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 5, "sim/multiplay/emesary/bridge[5]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 6, "sim/multiplay/emesary/bridge[6]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 7, "sim/multiplay/emesary/bridge[7]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 8, "sim/multiplay/emesary/bridge[8]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 9, "sim/multiplay/emesary/bridge[9]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 10, "sim/multiplay/emesary/bridge[10]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 11, "sim/multiplay/emesary/bridge[11]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 12, "sim/multiplay/emesary/bridge[12]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 13, "sim/multiplay/emesary/bridge[13]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 14, "sim/multiplay/emesary/bridge[14]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 15, "sim/multiplay/emesary/bridge[15]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 16, "sim/multiplay/emesary/bridge[16]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 17, "sim/multiplay/emesary/bridge[17]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 18, "sim/multiplay/emesary/bridge[18]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 19, "sim/multiplay/emesary/bridge[19]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 20, "sim/multiplay/emesary/bridge[20]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 21, "sim/multiplay/emesary/bridge[21]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 22, "sim/multiplay/emesary/bridge[22]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 23, "sim/multiplay/emesary/bridge[23]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 24, "sim/multiplay/emesary/bridge[24]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 25, "sim/multiplay/emesary/bridge[25]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 26, "sim/multiplay/emesary/bridge[26]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 27, "sim/multiplay/emesary/bridge[27]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 28, "sim/multiplay/emesary/bridge[28]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGE_BASE + 29, "sim/multiplay/emesary/bridge[29]", simgear::props::STRING, TT_ASIS, V1_1_2_PROP_ID, NULL, NULL},

        // To allow the bridge to identify itself and allow quick filtering based on type/ID.
        {EMESARYBRIDGETYPE_BASE + 0, "sim/multiplay/emesary/bridge-type[0]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 1, "sim/multiplay/emesary/bridge-type[1]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 2, "sim/multiplay/emesary/bridge-type[2]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 3, "sim/multiplay/emesary/bridge-type[3]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 4, "sim/multiplay/emesary/bridge-type[4]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 5, "sim/multiplay/emesary/bridge-type[5]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 6, "sim/multiplay/emesary/bridge-type[6]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 7, "sim/multiplay/emesary/bridge-type[7]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 8, "sim/multiplay/emesary/bridge-type[8]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 9, "sim/multiplay/emesary/bridge-type[9]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 10, "sim/multiplay/emesary/bridge-type[10]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 11, "sim/multiplay/emesary/bridge-type[11]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 12, "sim/multiplay/emesary/bridge-type[12]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 13, "sim/multiplay/emesary/bridge-type[13]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 14, "sim/multiplay/emesary/bridge-type[14]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 15, "sim/multiplay/emesary/bridge-type[15]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 16, "sim/multiplay/emesary/bridge-type[16]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 17, "sim/multiplay/emesary/bridge-type[17]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 18, "sim/multiplay/emesary/bridge-type[18]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 19, "sim/multiplay/emesary/bridge-type[19]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 20, "sim/multiplay/emesary/bridge-type[20]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 21, "sim/multiplay/emesary/bridge-type[21]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 22, "sim/multiplay/emesary/bridge-type[22]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 23, "sim/multiplay/emesary/bridge-type[23]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 24, "sim/multiplay/emesary/bridge-type[24]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 25, "sim/multiplay/emesary/bridge-type[25]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 26, "sim/multiplay/emesary/bridge-type[26]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 27, "sim/multiplay/emesary/bridge-type[27]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 28, "sim/multiplay/emesary/bridge-type[28]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {EMESARYBRIDGETYPE_BASE + 29, "sim/multiplay/emesary/bridge-type[29]", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},

        {FALLBACK_MODEL_ID, "sim/model/fallback-model-index", simgear::props::INT, TT_SHORTINT, V1_1_2_PROP_ID, NULL, NULL},
        {V2019_3_BASE, "sim/multiplay/comm-transmit-frequency-hz", simgear::props::INT, TT_INT, V1_1_2_PROP_ID, NULL, NULL},
        {V2019_3_BASE + 1, "sim/multiplay/comm-transmit-power-norm", simgear::props::INT, TT_SHORT_FLOAT_NORM, V1_1_2_PROP_ID, NULL, NULL},
        // Add new MP properties here
    };

    const unsigned int numProperties = (sizeof(sIdPropertyList) / sizeof(sIdPropertyList[0]));

    /* *********************************************************************************
     * Functions
     * *********************************************************************************/

    bool isSane(const FGExternalMotionData &motionInfo);
    const IdPropertyList *findProperty(unsigned id);
    bool verifyProperties(const xdr_data_t *data, const xdr_data_t *end);
    bool ProcessPosMsg(const MsgBuf &Msg, FGExternalMotionData *motionInfo);
    bool UnserializePosMsg(const MsgBuf &Msg, FGExternalMotionData *motionInfo);
    int SerializePosMsg(char *buffer, const FGExternalMotionData &motionInfo, int protocolToUse, std::string model, double timeAccel, std::string callSign);
    short get_scaled_short(double v, double scale);
    void FillMsgHdr(T_MsgHdr *MsgHdr, int MsgId, unsigned _len, std::string mCallsign);
    void SendMyPosition(const FGExternalMotionData &motionInfo, int protocolToUse, std::string model, double timeAccel, std::string callSign, int serverSocket, struct sockaddr *serverAddress);

}

#endif /* __ASAFG_PROCESSMSG_HPP__ */