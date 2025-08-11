#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sys/time.h>
#include <thread>
#include "asa/fg/Xdr.hpp"
#include <unistd.h>
#include <simgear/math/SGMath.hxx>
// Do not remove the include <boost/core/enable_if.hpp>. Need to avoid redefinition of structures by <simgear/props/props.hxx>
#include <boost/core/enable_if.hpp>
#include <simgear/props/props.hxx>
#include <sstream>
#include <vector>

#include "asa/fg/Utils.hpp"
#include "asa/fg/Xdrclient.hpp"
#include "asa/fg/Emesary.hpp"
#include "asa/fg/Factory.hpp"
#include "asa/fg/Processmsg.hpp"
#include "asa/fg/Mpmessages.hpp"
#include "asa/fg/ArmamentHitNotification.hpp"

#include "asa/core/base/Utils.hpp"
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

#include <cstdlib>
#include <chrono>

// FGPropertyData * createHitProp(int dist, std::string callSign);

asa::fg::FGExternalMotionData processMsg()
{
    std::cout << "#################################################" << std::endl;
    std::cout << "processMsg - inicio" << std::endl;
    std::cout << "#################################################\n";

    asa::fg::XDRClient client("127.0.0.1", 5000);


    // mensagem completa
    char buf[MAX_PACKET_SIZE];
    memset(buf, 0xff, MAX_PACKET_SIZE);
    //memcpy(buf, "\x46\x47\x46\x53\x00\x01\x00\x01\x00\x00\x00\x07\x00\x00\x01\x08\x64\x00\x00\x00\x00\x00\x00\x00\x58\x44\x52\x5f\x43\x4c\x49\x00\x41\x69\x72\x63\x72\x61\x66\x74\x2f\x66\x31\x36\x2f\x4d\x6f\x64\x65\x6c\x73\x2f\x46\x2d\x31\x36\x2e\x78\x6d\x6c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x41\x43\xc0\x83\xf1\x91\xce\x5b\xc1\x30\x73\x38\x2f\x60\xc5\x81\x41\x55\xc7\x8a\x0b\xc3\xd6\xec\x40\x03\x62\x2e\xc0\x03\x7c\x63\xbf\x1e\xbc\x4e\x3e\x9c\x0e\xbf\x3e\x9c\x0e\xbf\x3e\x9c\x0e\xbf\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1f\xac\xe0\x02\x32\xc8\x02\x04\x00\x00\x32\xc9\x00\x00\x00\x00\x32\xca\x00\x00\x00\x00\x2a\xf8\x00\x00\x00\x06\x00\x00\x2b\x20\x00\x00\x67\x00", 264);
    //memcpy(buf,"\x46\x47\x46\x53\x00\x01\x00\x01\x00\x00\x00\x07\x00\x00\x03\x0e\x00\x00\x01\x00\x00\x00\x00\x00\x46\x47\x56\x43\x47\x00\x00\x00\x41\x69\x72\x63\x72\x61\x66\x74\x2f\x66\x31\x36\x2f\x4d\x6f\x64\x65\x6c\x73\x2f\x46\x2d\x31\x36\x2e\x78\x6d\x6c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x40\xea\x17\x5a\xf7\x2b\xa7\x78\x3f\xb9\x99\x99\x99\x99\x99\x9a\x41\x43\xc3\x30\xf4\x5a\xc0\xdb\xc1\x30\x74\xee\x10\x2d\xa9\x39\x41\x55\xc6\xfc\x05\x0c\x52\x57\xc0\x4a\x34\x52\x3f\x14\x8a\xa1\x3e\xc4\xa1\x92\x43\x0d\xc8\x7c\xc0\x51\x40\x16\x41\x43\x5a\xfb\x3e\x1f\xde\x5a\xbc\x0a\xdd\x6f\x3c\x96\x8d\xf3\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1f\xac\xe0\x02\x32\xc8\x02\x04\x00\x00\x32\xc9\x00\x00\x00\x00\x32\xca\x00\x00\x00\x00\x2a\xf8\x00\x00\x00\x06\x00\x00\x2b\x20\x00\x00\x67\x00\x2e\xf3\x00\x16\x21\x83\x01\x01\x0d\x21\x96\x21\x87\xdd\x82\xf4\x83\x01\x0b\x88\x46\x47\x56\x43\x47\x7e\x32\xc8\x02\x04\x00\x00\x32\xc9\x00\x00\x00\x00\x32\xca\x00\x00\x00\x00\x2a\xf8\x00\x00\x00\x06\x00\x00\x2b\x20\x00\x00\x67\x00",322);
    //memcpy(buf,"\x46\x47\x46\x53\x00\x01\x00\x01\x00\x00\x00\x07\x00\x00\x02\xfa\x00\x00\x01\x00\x00\x00\x00\x00\x46\x47\x56\x43\x47\x00\x00\x00\x41\x69\x72\x63\x72\x61\x66\x74\x2f\x66\x31\x36\x2f\x4d\x6f\x64\x65\x6c\x73\x2f\x46\x2d\x31\x36\x2e\x78\x6d\x6c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x40\xef\xc6\xa3\x7a\x9d\xc4\xdf\x3f\xb9\x99\x99\x99\x99\x99\x9a\x41\x43\xc0\x83\xdd\x69\x17\xbd\xc1\x30\x73\x38\x10\x57\x67\x64\x41\x55\xc7\x89\xd4\xa9\xd2\x8e\xc0\x12\xd8\x63\xbf\xbd\x91\x1d\x3e\x95\xb8\x1a\x35\x0c\x54\x5d\x32\xc2\x26\xcb\xb5\xbf\x9d\xc4\x32\x6b\x0f\x35\xb4\x9a\xad\x5c\x2f\xe9\x0d\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1f\xac\xe0\x02\x00\x0a\x00\x02\x00\x64\x77\x09\x00\x65\x77\x09\x00\x66\x00\x00\x00\x67\xff\xb4\x00\x68\x00\x00\x00\x69\x00\x00\x00\x6a\x00\x00\x00\x6e\x00\x00\x00\x70\x00\x00\x00\xc8\x12\xe4\x00\xc9\x7f\xff\x00\xd2\x28\xac\x00\xd3\x7f\xff\x00\xdc\x27\xf1\x00\xdd\x7f\xff\x00\xe6\x00\x00\x00\xe7\x7f\xff\x00\xf0\x00\x00\x00\xf1\x7f\xff\x01\x2c\x01\x2c\x01\x2d\x02\x58\x03\x2a\x00\x00\x03\x2b\x00\x00\x03\x2c\x00\x00\x03\x2d\x00\x00\x03\x37\x00\x00\x03\xe9\x00\x00\x03\xea\x00\x00\x03\xeb\x00\x00\x03\xec\x27\x10\x03\xed\x03\xe8\x03\xee\x00\xa3\x04\xb1\x00\x00\x05\xdc\x04\xb0\x00\x00\x05\xdd\xff\xff\xd8\xf1\x05\xde\x00\x00\x05\xdf\x00\x01\x00\x00\x27\xd8\x3f\x80\x00\x00\x00\x00\x27\xda\x00\x00\x00\x00\x00\x00\x27\xdb\x40\x3d\xd3\x38\x00\x00\x27\xdc\xb4\x67\x9b\xab\x00\x00\x27\xdd\xbf\x80\x00\x00\x00\x00\x27\xde\x3f\x80\x00\x00\x00\x00\x27\xdf\xb7\xa0\xa1\x69\x00\x00\x27\xe0\x3f\x80\x00\x00\x00\x00\x27\xe1\x3f\x80\x00\x00\x00\x00\x27\xe2\x00\x00\x00\x00\x00\x00\x27\xe3\x00\x00\x00\x00\x00\x00\x27\xe4\x00\x00\x00\x00\x00\x00\x27\xe5\x00\x00\x00\x00\x00\x00\x27\xe6\x3c\xf2\xa9\x60\x00\x00\x27\xe7\x3a\x83\x12\x6f\x00\x00\x27\xe8\x3b\x92\x44\xb7\x00\x00\x27\xe9\x40\x60\xc6\xa1\x00\x00\x27\xeb\x00\x00\x00\x00\x00\x00\x27\xec\x3f\x80\x00\x00\x00\x00\x27\xed\x3a\x83\x12\x6f\x00\x00\x27\xee\x00\x00\x00\x00\x00\x00\x28\x3e\x00\x00\x00\x01\x00\x00\x28\x45\x00\x00\x00\x05\x00\x00\x28\x46\x00\x00\x00\x02\x04\x4d\x00\x07\x39\x33\x2d\x30\x35\x35\x31\x00\x00\x04\xb0\x00\x00\x00\x00\x00\x00\x05\x78\x00\x00\x00\x00\x05\xe0\x00\x00\x05\xe1\xd8\xf1\x27\x12\x00\x05\x48\x65\x6c\x6c\x6f\x27\x78\x00\x03\x34\x62\x36\x27\x79\x00\x20\x30\x39\x2d\x2d\x32\x30\x6d\x6d\x20\x43\x61\x6e\x6e\x6f\x6e\x2b\x2b\x30\x31\x2d\x2d\x32\x30\x6d\x6d\x20\x43\x61\x6e\x6e\x6f\x6e\x00\x00\x27\x7a\x00\x00\x00\x00\x27\x7b\x00\x03\x84\xb7\x44\x00\x00\x27\x7c\x00\x00\x00\x00\x29\x09\x00\x1e\x29\x0a\x00\x3c\x29\x0b\x00\x00\x2e\xd6\x00\x01\x00\x00\x2e\xf1\x00\x00\x00\x00\x00\x00\x2e\xf2\x00\x00\x00\x00\x00\x00\x2e\xf3\x00\x00\x00\x00\x32\xc8\x02\x04\x00\x00\x32\xc9\x00\x00\x00\x00\x32\xca\x00\x00\x00\x00\x2a\xf8\x00\x00\x00\x06\x00\x00\x2b\x20\x00\x00\x66\x00",762);
    //memcpy(buf,"\x46\x47\x46\x53\x00\x01\x00\x01\x00\x00\x00\x07\x00\x00\x03\x02\x00\x00\x01\x00\x00\x00\x00\x00\x46\x47\x56\x43\x47\x00\x00\x00\x41\x69\x72\x63\x72\x61\x66\x74\x2f\x66\x31\x36\x2f\x4d\x6f\x64\x65\x6c\x73\x2f\x46\x2d\x31\x36\x2e\x78\x6d\x6c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x40\xe9\xc7\x4a\xe8\x9e\x44\xdc\x3f\xb9\x99\x99\x99\x99\x99\x9a\x41\x43\xc0\x73\x62\x2c\xcb\x71\xc1\x30\x73\x87\xb8\x3a\xfd\xb9\x41\x55\xc7\x89\xc3\xf3\x4a\xa5\xc0\x0e\x92\x1c\xbf\xc8\x1e\x44\x3e\x8b\xa1\xc2\x35\x15\x00\xd9\x34\x95\x8b\xf1\xb5\xb5\xf1\x52\x34\x3e\x0c\x5d\xb4\xa7\x28\xbe\x32\x0d\x34\x65\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1f\xac\xe0\x02\x00\x0a\x00\x02\x00\x64\x77\x09\x00\x65\x77\x09\x00\x66\x00\x00\x00\x67\xff\xb5\x00\x68\x00\x00\x00\x69\x00\x00\x00\x6a\x00\x00\x00\x6e\x00\x00\x00\x70\x00\x00\x00\xc8\x12\xd6\x00\xc9\x7f\xff\x00\xd2\x28\x77\x00\xd3\x7f\xff\x00\xdc\x27\xc0\x00\xdd\x7f\xff\x00\xe6\x00\x00\x00\xe7\x7f\xff\x00\xf0\x00\x00\x00\xf1\x7f\xff\x01\x2c\x01\x2c\x01\x2d\x02\x58\x03\x2a\x00\x00\x03\x2b\x00\x00\x03\x2c\x00\x00\x03\x2d\x00\x00\x03\x37\x00\x00\x03\xe9\x00\x00\x03\xea\x00\x00\x03\xeb\x00\x00\x03\xec\x27\x10\x03\xed\x03\xe8\x03\xee\x00\x60\x04\xb1\x00\x00\x05\xdc\x04\xb0\x00\x00\x05\xdd\xff\xff\xd8\xf1\x05\xde\x00\x00\x05\xdf\x00\x01\x00\x00\x27\xd8\x3f\x80\x00\x00\x00\x00\x27\xda\x00\x00\x00\x00\x00\x00\x27\xdb\x40\x3d\xd3\x8c\x00\x00\x27\xdc\xb4\xd0\xaf\x12\x00\x00\x27\xdd\xbf\x80\x00\x00\x00\x00\x27\xde\x3f\x7f\xff\xfe\x00\x00\x27\xdf\x00\x00\x00\x00\x00\x00\x27\xe0\x3f\x80\x00\x00\x00\x00\x27\xe1\x3f\x80\x00\x00\x00\x00\x27\xe2\x00\x00\x00\x00\x00\x00\x27\xe3\x00\x00\x00\x00\x00\x00\x27\xe4\x00\x00\x00\x00\x00\x00\x27\xe5\x00\x00\x00\x00\x00\x00\x27\xe6\x3c\xdd\x59\xe0\x00\x00\x27\xe7\x3a\x83\x12\x6f\x00\x00\x27\xe8\x3b\x92\x44\xa5\x00\x00\x27\xe9\x40\x67\xb6\x17\x00\x00\x27\xeb\x00\x00\x00\x00\x00\x00\x27\xec\x3f\x80\x00\x00\x00\x00\x27\xed\x3a\x83\x12\x6f\x00\x00\x27\xee\x00\x00\x00\x00\x00\x00\x28\x3e\x00\x00\x00\x01\x00\x00\x28\x45\x00\x00\x00\x05\x00\x00\x28\x46\x00\x00\x00\x02\x04\x4d\x00\x07\x39\x33\x2d\x30\x35\x35\x31\x00\x00\x04\xb0\x00\x00\x00\x00\x00\x00\x05\x78\x00\x00\x00\x00\x05\xe0\x00\x00\x05\xe1\xd8\xf1\x27\x12\x00\x05\x48\x65\x6c\x6c\x6f\x27\x78\x00\x03\x39\x32\x36\x27\x79\x00\x28\x30\x38\x2d\x2d\x53\x6d\x6f\x6b\x65\x77\x69\x6e\x64\x65\x72\x20\x52\x65\x64\x2b\x2b\x30\x31\x2d\x2d\x53\x6d\x6f\x6b\x65\x77\x69\x6e\x64\x65\x72\x20\x52\x65\x64\x00\x00\x27\x7a\x00\x00\x00\x00\x27\x7b\x00\x03\x83\x7b\x95\x00\x00\x27\x7c\x00\x00\x00\x00\x29\x09\x00\x1e\x29\x0a\x00\x3c\x29\x0b\x00\x00\x2e\xd6\x00\x01\x00\x00\x2e\xf1\x00\x00\x00\x00\x00\x00\x2e\xf2\x00\x00\x00\x00\x00\x00\x2e\xf3\x00\x00\x00\x00\x32\xc8\x02\x04\x00\x00\x32\xc9\x00\x00\x00\x00\x32\xca\x00\x00\x00\x00\x2a\xf8\x00\x00\x1c\x06\x00\x00\x2b\x20\x00\x00\x66\x00",770);
    memcpy(buf,"\x46\x47\x46\x53\x00\x01\x00\x01\x00\x00\x00\x07\x00\x00\x02\xd1\x00\x00\x01\x00\x00\x00\x00\x00\x52\x31\x53\x4d\x52\x00\x00\x00\x41\x69\x72\x63\x72\x61\x66\x74\x2f\x66\x31\x36\x2f\x4d\x6f\x64\x65\x6c\x73\x2f\x46\x2d\x31\x36\x2e\x78\x6d\x6c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x40\xf0\x66\x79\xda\x64\xf5\x71\x3f\xb9\x99\x99\x99\x99\x99\x9a\x41\x43\xea\x64\x99\xa5\xae\xb2\xc1\x30\xb4\xb2\x5c\xe6\x2c\x56\x41\x55\xbc\xc6\x32\x0d\x32\xbe\xc0\x3b\x08\x83\x3f\x6f\x60\xa7\x3f\x20\x52\x09\x43\xd1\x5a\xd1\x3f\x55\x24\x77\x40\x72\x8e\xd2\xb9\x6a\x66\x2d\x38\x6f\x42\x81\xb9\x57\x57\xf6\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1f\xac\xe0\x02\x00\x0a\x00\x02\x00\x64\xf4\x1c\x00\x65\xf4\x17\x00\x66\x01\x83\x00\x67\xff\xe9\x00\x68\x00\x00\x00\x69\x00\x00\x00\x6a\x00\x00\x00\x6e\x00\x00\x00\x70\x00\x00\x00\xc8\x00\x00\x00\xc9\x00\x00\x00\xd2\x00\x00\x00\xd3\x00\x00\x00\xdc\x00\x00\x00\xdd\x00\x00\x00\xe6\x00\x00\x00\xe7\x00\x00\x00\xf0\x00\x00\x00\xf1\x00\x00\x01\x2c\x03\xe8\x01\x2d\x03\xe8\x03\x2a\x00\x00\x03\x2b\x00\x00\x03\x2c\x00\x00\x03\x2d\x00\x00\x03\x37\x00\x00\x03\xe9\x00\x00\x03\xea\x00\x00\x03\xeb\x00\x00\x03\xec\x00\x00\x03\xed\x03\xe8\x03\xee\x00\x00\x04\xb1\x00\x00\x05\xdc\x04\xb0\x00\x00\x05\xdd\xff\xff\xd8\xf1\x05\xde\x00\x00\x05\xdf\x00\x01\x00\x00\x27\xd8\x3f\x80\x00\x00\x00\x00\x27\xda\x00\x00\x00\x00\x00\x00\x27\xdb\x44\x3d\xb9\x65\x00\x00\x27\xdc\xbc\x41\xfd\x9e\x00\x00\x27\xdd\x3d\xcc\xa4\x86\x00\x00\x27\xde\xbd\xcc\xf5\x13\x00\x00\x27\xdf\xb4\x80\x00\x80\x00\x00\x27\xe0\x3f\x80\x00\x00\x00\x00\x27\xe1\x3f\x80\x00\x00\x00\x00\x27\xe2\x00\x00\x00\x00\x00\x00\x27\xe3\x00\x00\x00\x00\x00\x00\x27\xe4\x00\x00\x00\x00\x00\x00\x27\xe5\x00\x00\x00\x00\x00\x00\x27\xe6\x3b\xea\xbc\x40\x00\x00\x27\xe7\x3a\x83\x12\x6f\x00\x00\x27\xe8\x3f\x9e\x3d\x62\x00\x00\x27\xe9\x3f\x08\xc8\xaa\x00\x00\x27\xeb\x3f\x80\x00\x00\x00\x00\x27\xec\x3f\x80\x00\x00\x00\x00\x27\xed\x3a\x83\x12\x6f\x00\x00\x27\xee\x3c\x6a\xbc\x40\x00\x00\x28\x3e\x00\x00\x00\x00\x00\x00\x28\x45\x00\x00\x00\x05\x00\x00\x28\x46\x00\x00\x00\x02\x04\x4d\x00\x07\x39\x33\x2d\x30\x35\x35\x33\x00\x00\x04\xb0\x00\x00\x00\x00\x00\x00\x05\x78\x00\x00\x00\x00\x05\xe0\x00\x00\x05\xe1\xd8\xf1\x27\x12\x00\x05\x48\x65\x6c\x6c\x6f\x27\x78\x00\x03\x38\x38\x38\x27\x79\x00\x0f\x30\x37\x2d\x2d\x2b\x2b\x30\x30\x2d\x2d\x45\x6d\x70\x74\x79\x00\x00\x27\x7a\x00\x00\x00\x00\x27\x7b\x00\x03\x83\x75\xe1\x00\x00\x27\x7c\x00\x00\x00\x00\x29\x09\x00\x64\x29\x0a\x00\x64\x29\x0b\x00\x00\x2e\xd6\x00\x01\x32\xc8\x02\x04\x00\x00\x32\xc9\x00\x00\x00\x00\x32\xca\x00\x00\x00\x00\x2a\xf8\x00\x00\x00\x06\x00\x00\x2b\x20\x00\x00\x67\x00",721);
    
    //memcpy(buf,"\x46\x47\x46\x53\x00\x01\x00\x01\x00\x00\x00\x07\x00\x00\x03\x0e\x00\x00\x01\x00\x00\x00\x00\x00\x46\x47\x56\x43\x47\x00\x00\x00\x41\x69\x72\x63\x72\x61\x66\x74\x2f\x66\x31\x36\x2f\x4d\x6f\x64\x65\x6c\x73\x2f\x46\x2d\x31\x36\x2e\x78\x6d\x6c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x40\xea\x17\x5a\xf7\x2b\xa7\x78\x3f\xb9\x99\x99\x99\x99\x99\x9a\x41\x43\xc3\x30\xf4\x5a\xc0\xdb\xc1\x30\x74\xee\x10\x2d\xa9\x39\x41\x55\xc6\xfc\x05\x0c\x52\x57\xc0\x4a\x34\x52\x3f\x14\x8a\xa1\x3e\xc4\xa1\x92\x43\x0d\xc8\x7c\xc0\x51\x40\x16\x41\x43\x5a\xfb\x3e\x1f\xde\x5a\xbc\x0a\xdd\x6f\x3c\x96\x8d\xf3\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1f\xac\xe0\x02\x32\xc8\x02\x04\x00\x00\x32\xc9\x00\x00\x00\x00\x32\xca\x00\x00\x00\x00\x2a\xf8\x00\x00\x00\x06\x00\x00\x2b\x20\x00\x00\x67\x00",264);
    asa::fg::MsgBuf msg;
    //int msgLen = sizeof(msg.Msg);
    memcpy(msg.Msg, buf, sizeof(msg.Msg));
    asa::fg::FGExternalMotionData motionInfo;
    if (!ProcessPosMsg(msg, &motionInfo))
    {
        std::cerr << "Error processing message" << std::endl;
    }
    std::cout << "#################################################" << std::endl;
    std::cout << "processMsg - fim" << std::endl;
    std::cout << "#################################################\n";

    const asa::fg::IdPropertyList *plist = asa::fg::findProperty(1101);

    for (std::size_t i = 0; i < motionInfo.properties.size(); ++i) {
        std::cout << motionInfo.properties[i]->id << ", ";
        if(motionInfo.properties[i]->type == simgear::props::INT || motionInfo.properties[i]->type == simgear::props::BOOL){
            std::cout << motionInfo.properties[i]->int_value << std::endl; 
        }
        else if(motionInfo.properties[i]->type == simgear::props::FLOAT){
            std::cout << motionInfo.properties[i]->float_value << std::endl; 
        }
        else if(motionInfo.properties[i]->type == simgear::props::STRING){
            std::cout << motionInfo.properties[i]->string_value << std::endl; 
        }
    }
    return motionInfo;
}

bool envia(asa::fg::XDRClient *client, asa::fg::FGExternalMotionData &motionInfo, char *model, char *callsign)
{

    char ret[MAX_PACKET_SIZE];
    memset(ret, 0xff, MAX_PACKET_SIZE);

    int len = asa::fg::XDRClient::toBuf(ret, motionInfo, asa::fg::MAX_MP_PROTOCOL_VERSION, model, 1, callsign);

    return client->sendBuffer(ret, len);
}

void testXDR()
{
    char hit[500] = "\x2e\xf3\x00\x16\x21\x83\x01\x01\x0d\x21\x96\x21\x87\xdd\x82\xf4\x83\x01\x0b\x88\x46\x47\x56\x43\x47\x7e\x32\xc8\x02\x04\x00\x00\x32\xc9\x00\x00\x00\x00\x32\xca\x00\x00\x00\x00\x2a\xf8\x00\x00\x00\x06\x00\x00\x2b\x20\x00\x00\x67\x00";
    
    // usleep(15 * 1000 * 1000);

    // teste();
    // return 0;

    // https://wiki.flightgear.org/Multiplayer_protocol
    asa::fg::XDRClient client("172.18.105.126", 5000);//("172.16.249.163", 5000);

    // Create a thread for the receive() method
    std::thread receiveThread(&asa::fg::XDRClient::receive, &client);

    // Main loop for sending data

    float time = 0;
    float dt = 0.0;
    float ds = 0.f;

    // 2588938.745452,-1078062.662813,5709351.743036
    float x = 2588938.745452 + ds;
    float y = -1078062.662813 + ds;
    float z = 5709351.743036 + ds;

    // SGGeodesy::SGCartToGeod(cart, geo);

    float vu = 1.;
    float vv = 1.;
    float vw = 1.;
    int counter = 0;

    // srand(std::clock());

    // char callSign[6] = "XDR_X";
    // callSign[4] = 65 + rand() % (90 - 65);

    while (true)
    {

        time += dt;
        //  2588915.983094 -1078114.712775 57093652.123199 63.991753 -22.608544 146.906627 -2.230298 -1.560179 0.273225

        int extra_len = 0;

        if ((counter % 200) == 0 && counter != 0)
        {
            extra_len = 58;
        }

        client.send("Aircraft/f16/Models/F-16.xml",
                    std::array<double, 3>{x, y, z}.data(),                          //  currentPosition
                    std::array<float, 3>{-2.130530f, -1.660662f, 0.242749f}.data(), // currentOrientation
                    std::array<float, 3>{vu, vv, vw}.data(),                        // linearVel
                    std::array<float, 3>{0.0, 0.0, 0.0}.data(),                     // angularVel
                    std::array<float, 3>{0.0, 0.0, 0.0}.data(),                     // linearAccel
                    std::array<float, 3>{0.0, 0.0, 0.0}.data(),                     // angularAccel
                    "XDR_CLI",
                    nullptr,
                    hit, extra_len);

        //std::cout << "Message sent: " << counter << std::endl;
        // Add a delay if needed to control the sending rate
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10 Hz delay
        // std::cout << time * vu << std::endl;
        counter++;
    }

    // This line will never be reached, but it's here for completeness
    receiveThread.join();
}

asa::fg::FGPropertyData *testFactoryArmamentInFlightNotification()
{
    std::cout << "#################################################" << std::endl;
    std::cout << "FactoryArmamentInFlightNotification - inicio" << std::endl;
    std::cout << "#################################################\n"
              << std::endl;
    asa::fg::geoCoord c{63.977129, -22.60541199999999, 250};
    int kind = 2;
    int secondary_kind = 73;
    double u_fps = -2484.472049689441 + 3348;
    double heading = 178.64;
    double pitch = 15.94202898550725;
    std::string call_sign = std::string("FGSMR");
    int flag = 3;
    int unique_id = -99;
    asa::fg::Factory *fac = asa::fg::Factory::getInstance();

    asa::fg::FGPropertyData *msg = fac->FactoryArmamentInFlightNotification(c, kind, secondary_kind, u_fps, heading, pitch, call_sign,
                                                                            flag, unique_id, 0.);

    std::cout << "#################################################" << std::endl;
    std::cout << "FactoryArmamentInFlightNotification - fim" << std::endl;
    std::cout << "#################################################\n"
              << std::endl;
    return msg;
}

asa::fg::FGExternalMotionData *testFactoryPositon(float t, float lat, float lon, float elevation, float pitch, float roll, float heading, float vu, float vv, float vw, float vAngP, float vAngQ, float vAngR)
{
    std::cout << "#################################################" << std::endl;
    std::cout << "testeFactoryPositon - inicio" << std::endl;
    std::cout << "#################################################\n"
              << std::endl;

    // float time = 0;

    // // 2588938.745452,-1078062.662813,5709351.743036
    // float lat = 63.99173602244192;
    // float lon = -22.60758502664575;
    // float elevation = 150;

    // float pitch = 0;
    // float roll = 0;
    // float heading = 86;



    // float vu = 1.;
    // float vv = 1.;
    // float vw = 1.;
    asa::fg::Factory *fac = asa::fg::Factory::getInstance();

    asa::fg::FGExternalMotionData *motionInfo = new asa::fg::FGExternalMotionData;
    motionInfo = fac->HeaderAndAircraftPosition(t, 0,
                                                lat, lon, elevation,
                                                heading, pitch, roll,
                                                vu, vv, vw,
                                                vAngP, vAngQ, vAngR,
                                                0, 0, 0,
                                                0, 0, 0);
    std::cout << "#################################################" << std::endl;
    std::cout << "testeFactoryPositon - fim" << std::endl;
    std::cout << "#################################################\n"
              << std::endl;
    return motionInfo;
}

asa::fg::FGPropertyData *testFactoryArmamentHitNotification(std::string remoteCallSign)
{
    std::cout << "#################################################" << std::endl;
    std::cout << "FactoryArmamentHitNotification - inicio" << std::endl;
    std::cout << "#################################################\n"
              << std::endl;

    int kind = 4;
    int secondary_kind = 90;
    double relativeAltitude = -1.1;
    double distance = 0;
    double bearing = 184.44;

    asa::fg::Factory *fac = asa::fg::Factory::getInstance();

    asa::fg::FGPropertyData *msg = fac->FactoryArmamentHitNotification(kind, secondary_kind, relativeAltitude, distance, bearing, remoteCallSign,0);

    std::cout << "#################################################" << std::endl;
    std::cout << "FactoryArmamentHitNotification - fim" << std::endl;
    std::cout << "#################################################\n"
              << std::endl;
    return msg;
}



void testEnvio()
{
    std::cout << "#################################################" << std::endl;
    std::cout << "testeEnvio - inicio" << std::endl;
    std::cout << "#################################################\n";
    int counter = 0;
    asa::fg::XDRClient client("172.18.105.126", 5000);//("172.16.249.163",5000);//("172.18.105.126", 5000);
    asa::fg::FGExternalMotionData *motionInfo;
    char model[MAX_MODEL_NAME_LEN] = "Aircraft/f16/Models/F-16.xml";
    char callsign[MAX_CALLSIGN_LEN] = "XDR_CLI";
    int freq = 200;

    while (true)
    {
        float time = 0;

        // 2588938.745452,-1078062.662813,5709351.743036
        float lat = 63.99173602244192;
        float lon = -22.60758502664575;
        float elevation = 170;

        float pitch = 80;
        float roll = 15;
        float heading = 275;



        float vu = 1.;
        float vv = 1.;
        float vw = 1.;
        motionInfo = testFactoryPositon(time, lat, lon, elevation, pitch, roll,heading,  vu, vv, vw,0,0,0);
        float hDeg, pDeg, rDeg;
        motionInfo->orientation.getEulerRad(hDeg, pDeg, rDeg);

        // if (o.heading < 0)
        // {
        //     o.heading += 360.;
        // }

        // pPdu->entityOrientation.theta = o.heading * mixr::base::angle::D2RCC; // heading ?
        // pPdu->entityOrientation.psi = o.pitch * mixr::base::angle::D2RCC;     // pitch
        // pPdu->entityOrientation.phi = o.roll * mixr::base::angle::D2RCC; 
     



        if (counter % freq == 0 && counter != 0)
        {
            asa::fg::FGPropertyData *msgHit = testFactoryArmamentHitNotification("FGVCG");
            motionInfo->properties.insert(motionInfo->properties.begin(), msgHit);
        }

        if (!envia(&client, *motionInfo, model, callsign))
        {
            std::cout << "Erro ao enviar Message #" << counter << "." << std::endl;
        }
        else
        {
            std::cout << "Message #" << counter << " sent." << std::endl;
        }

        delete motionInfo;

        // Add a delay if needed to control the sending rate
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10 Hz delay
        // std::cout << time * vu << std::endl;
        counter++;
    }
    std::cout << "#################################################" << std::endl;
    std::cout << "testeEnvio - fim" << std::endl;
    std::cout << "#################################################\n";
}

void testEnvio2(asa::fg::XDRClient *client, float t, float lat, float lon, float elevation, float pitch, float roll, float heading, float vu, float vv, float vw,float vAngP, float vAngQ, float vAngR, int counter){
    std::cout << "#################################################" << std::endl;
    std::cout << "testeEnvio2 - inicio" << std::endl;
    std::cout << "#################################################\n";
    asa::fg::FGExternalMotionData *motionInfo;
    char model[MAX_MODEL_NAME_LEN] = "Aircraft/f16/Models/F-16.xml";
    char callsign[MAX_CALLSIGN_LEN] = "XDR_CLI";

    motionInfo = testFactoryPositon(t, lat, lon, elevation, heading, pitch, roll, vu, vv, vw, vAngP, vAngQ, vAngR);

       

    if (!envia(client, *motionInfo, model, callsign))
    {
        std::cout << "Erro ao enviar Message #" << counter << "." << std::endl;
    }
    else
    {
        std::cout << "Message #" << counter << " sent." << std::endl;
    }

    delete motionInfo;

    // Add a delay if needed to control the sending rate
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 10 Hz delay
    // std::cout << time * vu << std::endl;


}

void testeDataAsaJSON(){
    std::cout << "#################################################" << std::endl;
    std::cout << "testeDataAsaJSON - inicio" << std::endl;
    std::cout << "#################################################\n";
    // Open and read the JSON file
    std::ifstream ifs("data.json");
    if (!ifs.is_open()) {
        std::cerr << "Could not open the file!" << std::endl;
    }
    std::string jsonContent((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    ifs.close();

    // Parse the JSON content
    rapidjson::Document document;
    if (document.Parse(jsonContent.c_str()).HasParseError()) {
        std::cerr << "Error parsing JSON!" << std::endl;
    }

    // Vectors to store values
    std::vector<double> time;
    std::vector<double> lat;
    std::vector<double> lon;
    std::vector<double> alt;
    std::vector<double> velU;
    std::vector<double> velV;
    std::vector<double> velW;
    std::vector<double> velAngP;
    std::vector<double> velAngQ;
    std::vector<double> velAngR;
    std::vector<double> roll;
    std::vector<double> pitch;
    std::vector<double> heading;

    // Extract and store "time" values
    if (document.HasMember("time") && document["time"].IsObject()) {
        for (rapidjson::Value::ConstMemberIterator itr = document["time"].MemberBegin(); itr != document["time"].MemberEnd(); ++itr) {
            time.push_back(itr->value.GetDouble());
        }
    } else {
        std::cerr << "Invalid or missing 'time' key" << std::endl;
    }

    // Extract and store "lat" values
    if (document.HasMember("lat") && document["lat"].IsObject()) {
        for (rapidjson::Value::ConstMemberIterator itr = document["lat"].MemberBegin(); itr != document["lat"].MemberEnd(); ++itr) {
            lat.push_back(itr->value.GetDouble());
        }
    } else {
        std::cerr << "Invalid or missing 'lat' key" << std::endl;
    }

    // Extract and store "lon" values
    if (document.HasMember("lon") && document["lon"].IsObject()) {
        for (rapidjson::Value::ConstMemberIterator itr = document["lon"].MemberBegin(); itr != document["lon"].MemberEnd(); ++itr) {
            lon.push_back(itr->value.GetDouble());
        }
    } else {
        std::cerr << "Invalid or missing 'lon' key" << std::endl;
    }

    // Extract and store "alt" values
    if (document.HasMember("alt") && document["alt"].IsObject()) {
        for (rapidjson::Value::ConstMemberIterator itr = document["alt"].MemberBegin(); itr != document["alt"].MemberEnd(); ++itr) {
            alt.push_back(itr->value.GetDouble());
        }
    } else {
        std::cerr << "Invalid or missing 'alt' key" << std::endl;
    }

    // Extract and store "velU" values
    if (document.HasMember("velU") && document["velU"].IsObject()) {
        for (rapidjson::Value::ConstMemberIterator itr = document["velU"].MemberBegin(); itr != document["velU"].MemberEnd(); ++itr) {
            velU.push_back(itr->value.GetDouble());
        }
    } else {
        std::cerr << "Invalid or missing 'velU' key" << std::endl;
    }

    // Extract and store "velV" values
    if (document.HasMember("velV") && document["velV"].IsObject()) {
        for (rapidjson::Value::ConstMemberIterator itr = document["velV"].MemberBegin(); itr != document["velV"].MemberEnd(); ++itr) {
            velV.push_back(itr->value.GetDouble());
        }
    } else {
        std::cerr << "Invalid or missing 'velV' key" << std::endl;
    }

    // Extract and store "velW" values
    if (document.HasMember("velW") && document["velW"].IsObject()) {
        for (rapidjson::Value::ConstMemberIterator itr = document["velW"].MemberBegin(); itr != document["velW"].MemberEnd(); ++itr) {
            velW.push_back(itr->value.GetDouble());
        }
    } else {
        std::cerr << "Invalid or missing 'velW' key" << std::endl;
    }


    // Extract and store "velAngP" values
    if (document.HasMember("velAngP") && document["velAngP"].IsObject()) {
        for (rapidjson::Value::ConstMemberIterator itr = document["velAngP"].MemberBegin(); itr != document["velAngP"].MemberEnd(); ++itr) {
            velAngP.push_back(itr->value.GetDouble());
        }
    } else {
        std::cerr << "Invalid or missing 'velAngP' key" << std::endl;
    }

    // Extract and store "velAngQ" values
    if (document.HasMember("velAngQ") && document["velAngQ"].IsObject()) {
        for (rapidjson::Value::ConstMemberIterator itr = document["velAngQ"].MemberBegin(); itr != document["velAngQ"].MemberEnd(); ++itr) {
            velAngQ.push_back(itr->value.GetDouble());
        }
    } else {
        std::cerr << "Invalid or missing 'velAngQ' key" << std::endl;
    }


        // Extract and store "velAngR" values
    if (document.HasMember("velAngR") && document["velAngR"].IsObject()) {
        for (rapidjson::Value::ConstMemberIterator itr = document["velAngR"].MemberBegin(); itr != document["velAngR"].MemberEnd(); ++itr) {
            velAngR.push_back(itr->value.GetDouble());
        }
    } else {
        std::cerr << "Invalid or missing 'velAngR' key" << std::endl;
    }

    // Extract and store "roll" values
    if (document.HasMember("roll") && document["roll"].IsObject()) {
        for (rapidjson::Value::ConstMemberIterator itr = document["roll"].MemberBegin(); itr != document["roll"].MemberEnd(); ++itr) {
            roll.push_back(itr->value.GetDouble());
        }
    } else {
        std::cerr << "Invalid or missing 'roll' key" << std::endl;
    }

    // Extract and store "pitch" values
    if (document.HasMember("pitch") && document["pitch"].IsObject()) {
        for (rapidjson::Value::ConstMemberIterator itr = document["pitch"].MemberBegin(); itr != document["pitch"].MemberEnd(); ++itr) {
            pitch.push_back(itr->value.GetDouble());
        }
    } else {
        std::cerr << "Invalid or missing 'pitch' key" << std::endl;
    }

    // Extract and store "heading" values
    if (document.HasMember("heading") && document["heading"].IsObject()) {
        for (rapidjson::Value::ConstMemberIterator itr = document["heading"].MemberBegin(); itr != document["heading"].MemberEnd(); ++itr) {
            heading.push_back(itr->value.GetDouble());
        }
    } else {
        std::cerr << "Invalid or missing 'heading' key" << std::endl;
    }

    asa::fg::XDRClient client("172.18.105.126", 5000);

    int j = 0;

    while(j<=100){

        float time = 0.0;
        float lat = 63.99173602244192;
        float lon = -22.60758502664575;
        float alt = 150;

        float pitch = 2.0;
        float roll = 0.0;
        float heading = 86.2;
        testEnvio2(&client,time, lat, lon, alt, pitch, roll, heading, 0, 0, 0,0, 0, 0, j);
        j++;
        std::cout << j << std::endl;
    }

    for (size_t i = 0; i < time.size(); ++i) {
        testEnvio2(&client,0, lat[i], lon[i], alt[i], pitch[i], roll[i], heading[i], velU[i], velV[i], velW[i], velAngP[i], velAngQ[i], velAngR[i], i);
    }
    std::cout << "#################################################" << std::endl;
    std::cout << "testeDataAsaJSON - fim" << std::endl;
    std::cout << "#################################################\n";
}

int main()
{
    std::cout << "#################################################" << std::endl;
    std::cout << "Main - inicio" << std::endl;
    std::cout << "#################################################\n";
    
    testEnvio();
    //testXDR();

    // asa::fg::FGExternalMotionData *motionInfo = testeFactoryPositon();
    // asa::fg::FGPropertyData *msgHit = testFactoryArmamentHitNotification("FGVCG");
    // motionInfo->properties.push_back(msgHit);
    
    // asa::fg::FGExternalMotionData motionInfoReal = processMsg();
    // asa::fg::FGExternalMotionData *m2 = testFactoryPositon();
    //processMsg();
    //testeDataAsaJSON();
    
    std::cout << "#################################################" << std::endl;
    std::cout << "Main - fim" << std::endl;
    std::cout << "#################################################\n";
    return 0;
}