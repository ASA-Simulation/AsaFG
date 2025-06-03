#include "asa/fg/Emesary.hpp"

namespace asa::fg
{

    std::string BinaryAsciiTransfer::encodeNumeric(double _num, int length, double factor)
    {
        long long num = (long long)(_num / factor);
        long long irange = (long long)(po2[length]);

        if (num < -irange)
            num = -irange;
        else if (num > irange)
            num = irange;

        num += irange;

        if (num == 0.)
            return empty_encoding.substr(0, length);

        std::string arr = "";
        while (num > 0. && length > 0)
        {
            double num0 = num;
            num = (long long)(num / _base);
            double rem = num0 - (num * _base);
            arr = alphabet.substr(rem, 1) + arr;
            length--;
        }

        if (length > 0)
            arr = spaces.substr(0, length) + arr;

        return arr;
    }

    ReturnValueDouble BinaryAsciiTransfer::decodeNumeric(const std::string &str, int length, double factor, int pos)
    {
        double irange = po2[length];
        double power = length - 1;
        ReturnValueDouble retval;
        retval.value = 0.;
        retval.pos = pos;

        while (length > 0 && power > 0)
        {
            char c = str[retval.pos];
            if (c != ' ')
            {
                break;
            }
            power--;
            length--;
            retval.pos++;
        }

        while (length >= 0 && power >= 0)
        {
            char c = str[retval.pos];
            if (c != ' ')
            {
                int cc = alphabet.find(c);
                if (cc < 0)
                {
                    std::cout << c << std::endl;
                    std::cout << "Emesary: BinaryAsciiTransfer.decodeNumeric: Bad encoding" << std::endl;
                    return retval;
                }
                retval.value += cc * std::pow(_base, power);
                power--;
            }
            length--;
            retval.pos++;
        }

        retval.value -= (double)irange;
        retval.value = retval.value * factor;
        return retval;
    }

    std::string BinaryAsciiTransfer::encodeInt(int num, int length)
    {
        return encodeNumeric((double)num, length, 1.0);
    }

    ReturnValueInt BinaryAsciiTransfer::decodeInt(const std::string &str, int length, int pos)
    {
        ReturnValueDouble r = decodeNumeric(str, length, 1.0, pos);
        return ReturnValueInt{(int)r.value, r.pos};
    }

    std::string BinaryAsciiTransfer::DefineAlphabet()
    {
        std::string concatenatedString;
        // excluded chars 32 (<space>), 33 (!), 35 (#), 36($), 126 (~), 127 (<del>)
        for (int i = 1; i <= 255; ++i)
        {
            // if (i > 127 || (i < 126 && i > 36) || i < 32 || i == 34)
            if (i != 32 && i != 33 && i != 35 && i != 36 && i != 126 && i != 127)
            {
                char ch = static_cast<char>(i);
                concatenatedString += ch;
            }
        }
        return concatenatedString;
    }

    char TransferString::getalphanumericchar(char v)
    {
        if (BinaryAsciiTransfer::alphabet.find(v) != std::string::npos)
            return v;
        return '\0';
    }

    std::string TransferString::encode(const std::string &v)
    {
        if (v.empty())
            return "0";

        int l = std::min(static_cast<int>(v.size()), MaxLength);
        std::string rv = "";
        int actual_len = 0;

        for (int ii = 0; ii < l; ii++)
        {
            char ev = getalphanumericchar(v[ii]);
            if (ev != '\0')
            {
                rv += ev;
                actual_len++;
            }
        }

        rv = BinaryAsciiTransfer::encodeNumeric(l, 1, 1.0) + rv;
        return rv;
    }

    ReturnValueString TransferString::decode(const std::string &v, int pos)
    {
        ReturnValueDouble dv = BinaryAsciiTransfer::decodeNumeric(v, 1, 1.0, pos);
        ReturnValueString dvstr{"", 0};
        int length = dv.value;

        if (length > 0)
        {
            std::string rv = v.substr(dv.pos, length);
            dv.pos += length;
            dvstr.pos = dv.pos;
            dvstr.value = rv;
        }
        return dvstr;
    }

    std::string TransferByte::encode(int v)
    {
        // Assuming BinaryAsciiTransfer.encodeNumeric() is a method that encodes 'v' into binary ASCII
        return BinaryAsciiTransfer::encodeNumeric(v, 1, 1.0);
    }

    ReturnValueDouble TransferByte::decode(std::string &v, int pos)
    {
        // Assuming BinaryAsciiTransfer.decodeNumeric() is a method that decodes 'v' from binary ASCII
        return BinaryAsciiTransfer::decodeNumeric(v, 1, 1.0, pos);
    }

    std::string TransferInt::encode(int v, int length)
    {
        return BinaryAsciiTransfer::encodeNumeric(double(v), length, 1.0);
    }

    ReturnValueDouble TransferInt::decode(std::string &v, int length, int pos)
    {
        return BinaryAsciiTransfer::decodeNumeric(v, length, 1.0, pos);
    }

    std::string TransferFixedDouble::encode(double v, int length, double factor)
    {
        return BinaryAsciiTransfer::encodeNumeric(v, length, factor);
    }

    ReturnValueDouble TransferFixedDouble::decode(std::string &v, int length, double factor, int pos)
    {
        return BinaryAsciiTransfer::decodeNumeric(v, length, factor, pos);
    }

    std::string TransferCoord::encode(const geoCoord &v)
    {
        std::string encodedLat = BinaryAsciiTransfer::encodeNumeric(v.lat, TransferCoord::LatLonLength, TransferCoord::LatLonFactor);
        std::string encodedLon = BinaryAsciiTransfer::encodeNumeric(v.lon, TransferCoord::LatLonLength, TransferCoord::LatLonFactor);
        std::string encodedAlt = TransferInt::encode(v.alt, TransferCoord::AltLength);

        return encodedLat + encodedLon + encodedAlt;
    }

    ReturnValueCoord TransferCoord::decode(std::string &v, int pos)
    {
        ReturnValueDouble dvLat = BinaryAsciiTransfer::decodeNumeric(v, TransferCoord::LatLonLength, TransferCoord::LatLonFactor, pos);
        ReturnValueDouble dvLon = BinaryAsciiTransfer::decodeNumeric(v, TransferCoord::LatLonLength, TransferCoord::LatLonFactor, dvLat.pos);
        ReturnValueDouble dvAlt = TransferInt::decode(v, TransferCoord::AltLength, dvLon.pos);

        geoCoord Coord{dvLat.value, dvLon.value, dvAlt.value};
        ReturnValueCoord decodedCoord{Coord, dvAlt.pos};
        return decodedCoord;
    }

    const std::string BinaryAsciiTransfer::alphabet = BinaryAsciiTransfer::DefineAlphabet();

    const std::vector<long long> BinaryAsciiTransfer::po2 = {1, 124, 30752, 7626496, 1891371008, 469060009984, 116326882476032, 28849066854055936};
    const int BinaryAsciiTransfer::_base = 248;
    const std::string BinaryAsciiTransfer::spaces = "                                  ";
    const std::string BinaryAsciiTransfer::empty_encoding = "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01";

    const double TransferCoord::LatLonFactor = 0.000001;
    const int TransferString::MaxLength = 16;

    double normdeg(double angle)
    {
        while (angle < 0.)
            angle += 360.;
        while (angle >= 360.)
            angle -= 360.;
        return angle;
    }

    double normdeg180(double angle)
    {
        while (angle <= -180.)
            angle += 360.;
        while (angle > 180.)
            angle -= 360.;
        return angle;
    }

}