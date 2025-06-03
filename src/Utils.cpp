#include <iostream>
#include <string>
#include <cmath>


#include "asa/fg/Utils.hpp"
#include "asa/fg/Processmsg.hpp"

namespace asa::fg
{
    std::string strProp(simgear::props::Type t)
    {
        switch (t)
        {
        case simgear::props::Type::NONE:
            return "NONE";
        case simgear::props::Type::ALIAS:
            return "ALIAS";
        case simgear::props::Type::BOOL:
            return "BOOL";
        case simgear::props::Type::INT:
            return "INT";
        case simgear::props::Type::LONG:
            return "LONG";
        case simgear::props::Type::FLOAT:
            return "FLOAT";
        case simgear::props::Type::DOUBLE:
            return "DOUBLE";
        case simgear::props::Type::STRING:
            return "STRING";
        case simgear::props::Type::UNSPECIFIED:
            return "UNSPECIFIED";
        case simgear::props::Type::EXTENDED:
            return "EXTENDED";
        // Extended properties
        case simgear::props::Type::VEC3D:
            return "VEC3D";
        case simgear::props::Type::VEC4D:
            return "VEC4D";
        }
        return "UNDEFINED";
    }

    void sprintProperty(std::stringstream &ss, FGPropertyData *p)
    {
        switch (p->type)
        {
        case simgear::props::BOOL:
            ss << p->id << ", " << strProp(p->type) << ", " << p->int_value << std::endl;
            break;
        case simgear::props::INT:
            ss << p->id << ", " << strProp(p->type) << ", " << p->int_value << std::endl;
            break;
        case simgear::props::LONG:
            ss << p->id << ", " << strProp(p->type) << ", " << p->int_value << std::endl;
            break;
        case simgear::props::FLOAT:
            ss << p->id << ", " << strProp(p->type) << ", " << p->float_value << std::endl;
            break;
        case simgear::props::DOUBLE:
            ss << p->id << ", " << strProp(p->type) << ", " << p->float_value << std::endl;
            break;
        case simgear::props::STRING:
        {
            ss << p->id << ", " << strProp(p->type) << ", " << p->string_value << ", "; // << std::endl;
            std::ios oldState(nullptr);
            oldState.copyfmt(ss);
            int i;
            for (i = 0; p->string_value[i] != 0; ++i)
            {
                ss << "0x" << std::setw(2) << std::setfill('0') << std::hex << (int)(p->string_value[i] & 0xff) << ' ';
            }
            ss.copyfmt(oldState);
            ss << "#" << i << std::endl;
        }
        break;
        default:
            ss << p->id << ", " << strProp(p->type) << std::endl;
            break;
        }
    }

    asa::fg::FGPropertyData* findPropertyMotionInfo (FGExternalMotionData *motionInfo, unsigned int id){
        for (const auto& item : motionInfo->properties) {
            if (item->id == id) {
                return item;
            }
        }
        return nullptr;
    }

    asa::fg::FGPropertyData* createProperty (int id){
        FGPropertyData* pData = new FGPropertyData;
        pData->id = id;
        const asa::fg::IdPropertyList *plist = asa::fg::findProperty(id);
        pData->type =  plist->type;
        pData->int_value = 0;
        pData->float_value = 0.0;
        pData->string_value = nullptr;
        return pData;
    }

    void setSmoke(FGExternalMotionData *motionInfo, int value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,11010);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(11010);
            motionInfo->properties.push_back(pData);   
        }
        pData->int_value = value;
    }
    
    void setSmokeGreen(FGExternalMotionData *motionInfo, int value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,11013);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(11013);
            motionInfo->properties.push_back(pData);   
        }
        pData->int_value = value;

        pData = findPropertyMotionInfo(motionInfo,11014);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(11014);
            motionInfo->properties.push_back(pData);   
        }
        pData->int_value = value;

    }
    
    void setSmokeRed(FGExternalMotionData *motionInfo, int value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,11012);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(11012);
            motionInfo->properties.push_back(pData);   
        }
        pData->int_value = value;

        pData = findPropertyMotionInfo(motionInfo,11011);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(11011);
            motionInfo->properties.push_back(pData);   
        }
        pData->int_value = value;
    }
    
    void setSmokeBlue(FGExternalMotionData *motionInfo, int value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,11016);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(11016);
            motionInfo->properties.push_back(pData);   
        }
        pData->int_value = value;

        pData = findPropertyMotionInfo(motionInfo,11015);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(11015);
            motionInfo->properties.push_back(pData);   
        }
        pData->int_value = value;

    }
    
    void setSmokeWhite(FGExternalMotionData *motionInfo, int value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,11018);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(11018);
            motionInfo->properties.push_back(pData);   
        }
        pData->int_value = value;

        pData = findPropertyMotionInfo(motionInfo,11019);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(11019);
            motionInfo->properties.push_back(pData);   
        }
        pData->int_value = value;
    }
    
    void setLivery(FGExternalMotionData *motionInfo, std::string value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,1101);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(1101);
            motionInfo->properties.push_back(pData);   
        }
        char* valueChar = new char[value.length() + 1];
        pData->string_value = std::strcpy(valueChar, value.c_str());
        
    }
    
    void setSpeed(FGExternalMotionData *motionInfo, float value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,10203);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(10203);
            motionInfo->properties.push_back(pData);   
        }
        pData->float_value = value;

    }
    
    void setMach(FGExternalMotionData *motionInfo, float value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,10216);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(10216);
            motionInfo->properties.push_back(pData);   
        }
        pData->float_value = value;
        
    }
    
    void setStr4Iff(FGExternalMotionData *motionInfo, std::string value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,10104);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(10104);
            motionInfo->properties.push_back(pData);   
        }
        char* valueChar = new char[value.length() + 1];
        pData->string_value = std::strcpy(valueChar, value.c_str());

    }
    
    void setStr5Pylons(FGExternalMotionData *motionInfo, std::string value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,10105);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(10105);
            motionInfo->properties.push_back(pData);   
        }
        char* valueChar = new char[value.length() + 1];
        pData->string_value = std::strcpy(valueChar, value.c_str());
        
    }
    
    void setStr6Rwr(FGExternalMotionData *motionInfo, std::string value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,10106);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(10106);
            motionInfo->properties.push_back(pData);   
        }
        char* valueChar = new char[value.length() + 1];
        pData->string_value = std::strcpy(valueChar, value.c_str());
        
    }
    
    void setStr7DataLink(FGExternalMotionData *motionInfo, std::string value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,10107);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(10107);
            motionInfo->properties.push_back(pData);   
        }
        char* valueChar = new char[value.length() + 1];
        pData->string_value = std::strcpy(valueChar, value.c_str());
                
    }
    
    void setStr8DataLink(FGExternalMotionData *motionInfo, std::string value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,10108);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(10108);
            motionInfo->properties.push_back(pData);   
        }
        char* valueChar = new char[value.length() + 1];
        pData->string_value = std::strcpy(valueChar, value.c_str());

    }
    
    void setGearUp(FGExternalMotionData *motionInfo, int value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,200);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(200);
            motionInfo->properties.push_back(pData);   
        }
        pData->float_value = value;

        pData = findPropertyMotionInfo(motionInfo,201);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(201);
            motionInfo->properties.push_back(pData);   
        }
        pData->float_value = value;

        pData = findPropertyMotionInfo(motionInfo,210);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(210);
            motionInfo->properties.push_back(pData);   
        }
        pData->float_value = value;

        pData = findPropertyMotionInfo(motionInfo,211);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(211);
            motionInfo->properties.push_back(pData);   
        }
        pData->float_value = value;

        pData = findPropertyMotionInfo(motionInfo,220);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(220);
            motionInfo->properties.push_back(pData);   
        }
        pData->float_value = value;

        pData = findPropertyMotionInfo(motionInfo,221);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(221);
            motionInfo->properties.push_back(pData);   
        }
        pData->float_value = value;

        pData = findPropertyMotionInfo(motionInfo,230);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(230);
            motionInfo->properties.push_back(pData);   
        }
        pData->float_value = value;

        pData = findPropertyMotionInfo(motionInfo,231);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(231);
            motionInfo->properties.push_back(pData);   
        }
        pData->float_value = value;

        pData = findPropertyMotionInfo(motionInfo,240);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(230);
            motionInfo->properties.push_back(pData);   
        }
        pData->float_value = value;

        pData = findPropertyMotionInfo(motionInfo,241);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(231);
            motionInfo->properties.push_back(pData);   
        }
        pData->float_value = value;


    }

    void setInFligthNotification(FGExternalMotionData *motionInfo, std::string value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,12018);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(12018);
            motionInfo->properties.push_back(pData); 
        }
        pData->string_value = new char[value.length() + 1];
        std::strcpy(pData->string_value, value.c_str());
    }

    void setHitNotification(FGExternalMotionData *motionInfo, std::string value){
        FGPropertyData* pData = findPropertyMotionInfo(motionInfo,12019);
        if(pData==nullptr){
            FGPropertyData* pData = createProperty(12019);
            motionInfo->properties.push_back(pData); 
        }
        pData->string_value = new char[value.length() + 1];
        std::strcpy(pData->string_value, value.c_str());
    }



    // Function to get the element of a 3x3 matrix using a linear index
    double getElement(const std::vector<double>& matrix, int rows, int cols, int i, int j) {
        return matrix[i * cols + j];
    }

    // Function to set the element of a 3x3 matrix using a linear index
    void setElement(std::vector<double>& matrix, int rows, int cols, int i, int j, double value) {
        matrix[i * cols + j] = value;
    }

    // Function to multiply two 3x3 matrices
    std::vector<double> multiplyMatrices(const std::vector<double>& A, const std::vector<double>& B, int rows, int cols) {
        std::vector<double> C(rows * cols, 0.0);

        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                for (int k = 0; k < cols; ++k) {
                    double value = getElement(C, rows, cols, i, j) +
                                getElement(A, rows, cols, i, k) * getElement(B, rows, cols, k, j);
                    setElement(C, rows, cols, i, j, value);
                }
            }
        }
        return C;
    }

    // Function to multiply a 3x3 matrix with a 3x1 vector
    std::vector<double> multiplyMatrixVector(const std::vector<double>& A, const std::vector<double>& v, int rows, int cols) {
        std::vector<double> result(rows, 0.0);

        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                result[i] += getElement(A, rows, cols, i, j) * v[j];
            }
        }
        return result;
    }

}