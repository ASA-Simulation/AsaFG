#ifndef __asa_fg_emesary_H__
#define __asa_fg_emesary_H__

#include <iostream>
#include <cmath>
#include <vector>
#include <sys/time.h>
#include <unistd.h>

namespace asa::fg
{   
    // #==================================================================
    // #                       Notification Kinds
    // #==================================================================
    // # Static variables for notification.Kind:
    #define CREATE      1
    #define MOVE        2
    #define DESTROY     3
    #define IMPACT      4
    #define REQUEST_ALL 5

    struct ReturnValueDouble
    {
        double value;
        int pos;
    };

    struct ReturnValueInt
    {
        int value;
        int pos;
    };

    struct ReturnValueString
    {
        std::string value;
        int pos;
    };

    struct geoCoord
    {
        double lat;
        double lon;
        double alt;
    };

    struct ReturnValueCoord
    {
        geoCoord value;
        int pos;
    };

    class BinaryAsciiTransfer
    {
    public:
        static const std::string alphabet;
        static const std::vector<long long> po2;
        static const int _base;
        static const std::string spaces;
        static const std::string empty_encoding;

        static std::string encodeNumeric(double _num, int length, double factor);
        static ReturnValueDouble decodeNumeric(const std::string &str, int length, double factor, int pos);
        static std::string encodeInt(int num, int length);
        static ReturnValueInt decodeInt(const std::string &str, int length, int pos);
        static std::string DefineAlphabet();
    };

    class TransferString
    {
    public:
        static const int MaxLength;

        static char getalphanumericchar(char v);
        static std::string encode(const std::string &v);
        static ReturnValueString decode(const std::string &v, int pos);
    };

    class TransferByte
    {
    public:
        static std::string encode(int v);
        static ReturnValueDouble decode(std::string &v, int pos);
    };

    class TransferInt
    {
    public:
        static std::string encode(int v, int length);
        static ReturnValueDouble decode(std::string &v, int length, int pos);
    };

    class TransferFixedDouble
    {
    public:
        static std::string encode(double v, int length, double factor);
        static ReturnValueDouble decode(std::string &v, int length, double factor, int pos);
    };

    class TransferCoord
    {
    public:
        static const int LatLonLength = 4;
        static const double LatLonFactor;
        static const int AltLength = 3;

        static std::string encode(const geoCoord &v);
        static ReturnValueCoord decode(std::string &v, int pos);
    };

    double normdeg(double angle);

    double normdeg180(double angle);

}

#endif /* __asa_fg_emesary_H__ */