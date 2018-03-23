
#include <stdlib.h>
#include "IMetaData.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#define STHROW(msg) { \
     ostringstream err;\
     err<<msg; \
     throw runtime_error(err.str()); }

namespace eve {

IMetaData::IMetaData() : datatype(DTunknown), devtype(Unknown),
    dstype(EVEDSTUnknown)
{
    dim0 = 0;
    dim1 = 0;
    h5dimensions[0]=0;
    h5dimensions[1]=0;
    datatype = DTunknown;
    devtype = Unknown;
    dstype = EVEDSTUnknown;
}

IMetaData::IMetaData(string basep, string calc, string h5n, Section section, map<string, string> attrib)
    : selSection(section), path(basep), calculation(calc), h5name(h5n), attributes(attrib)
{

    dim0 = 0;
    dim1 = 0;
    h5dimensions[0]=0;
    h5dimensions[1]=0;
    datatype = DTunknown;
    devtype = Unknown;
    dstype = EVEDSTUnknown;

    if (attributes.count("axis") > 0)
        xmlId = attributes.find("axis")->second;
    // overwrites xmlId if present
    if (attributes.count("XML-ID") > 0)
        xmlId = attributes.find("XML-ID")->second;
    if (attributes.count("channel") > 0)
        channelId = attributes.find("channel")->second;
    if (attributes.count("normalizeId") > 0)
        normalizeId = attributes.find("normalizeId")->second;
    if (attributes.count("Name") > 0)
        name = attributes.find("Name")->second;

    if (xmlId.empty()) xmlId = h5name;

    if (attributes.count("DeviceType") > 0) {
        string devtypestr = attributes.find("DeviceType")->second;
        if (devtypestr.compare("Axis") == 0)
            devtype = Axis;
        else if (devtypestr.compare("Channel") == 0)
            devtype = Channel;
    }
    if (selSection == Timestamp) {
        devtype = Channel;
        name = "PosCountTimer";
    }

}

string IMetaData::getFQH5Name(){

    string calcstring = "";
    if (calculation.length() > 0) calcstring = calculation + "/";
    return path + calcstring + h5name;
}

string IMetaData::getUnit(){

    if (attributes.count("Unit") > 0)
        return attributes.find("Unit")->second;
     else
        return string("");
}

void IMetaData::setDataType(DataSet& ds){

    try {
        DataSpace dspace = ds.getSpace();
        int rank = dspace.getSimpleExtentNdims();
        if (rank <= 2) {
            dspace.getSimpleExtentDims(h5dimensions);
            dim1 = h5dimensions[0];
        }
        switch (ds.getTypeClass()){
        case H5T_INTEGER:
            dstype = EVEDSTArray;
            if (ds.getIntType() == PredType::NATIVE_INT8) datatype = DTint8;
            else if (ds.getIntType() == PredType::NATIVE_UINT8) datatype = DTuint8;
            else if (ds.getIntType() == PredType::NATIVE_INT16) datatype = DTint16;
            else if (ds.getIntType() == PredType::NATIVE_UINT16) datatype = DTuint16;
            else if (ds.getIntType() == PredType::NATIVE_UINT32) datatype = DTuint32;
            else if (ds.getIntType() == PredType::NATIVE_INT32) datatype = DTint32;
            break;
        case H5T_FLOAT:
            dstype = EVEDSTArray;
            if (ds.getFloatType()  == PredType::NATIVE_FLOAT) datatype = DTfloat32;
            else if (ds.getFloatType() == PredType::NATIVE_DOUBLE) datatype = DTfloat64;
            break;
        case H5T_STRING:
            dstype = EVEDSTArray;
            datatype = DTstring;
            break;
        case H5T_COMPOUND:
        {
            CompType compdt = ds.getCompType();
            int count = compdt.getNmembers();
            dim0 = h5dimensions[0];
            dim1 = 0;
            for(int idx = 0; idx < count; ++idx){
                H5::DataType memberDtyp = compdt.getMemberDataType(idx);
                int H5class = memberDtyp.getClass();
                if ((idx == 0) && ((compdt.getMemberName(idx).compare("PosCounter") == 0) ||
                                   (compdt.getMemberName(idx).compare("mSecsSinceStart") == 0))){
                    if (count == 2) {
                        dstype = EVEDSTPCOneColumn;
                    }
                    else if (count == 3) {
                        dstype = EVEDSTPCTwoColumn;
                    }
                }
                else {
                    if (H5class == H5T_INTEGER){
                        if (memberDtyp == PredType::NATIVE_INT8) datatype = DTint8;
                        else if (memberDtyp == PredType::NATIVE_UINT8) datatype = DTuint8;
                        else if (memberDtyp == PredType::NATIVE_INT16) datatype = DTint16;
                        else if (memberDtyp == PredType::NATIVE_UINT16) datatype = DTuint16;
                        else if (memberDtyp == PredType::NATIVE_UINT32) datatype = DTuint32;
                        else if (memberDtyp == PredType::NATIVE_INT32) datatype = DTint32;
                    }
                    else if (H5class == H5T_FLOAT){
                        if (memberDtyp == PredType::NATIVE_FLOAT) datatype = DTfloat32;
                        else if (memberDtyp == PredType::NATIVE_DOUBLE) datatype = DTfloat64;
                    }
                    else if (H5class == H5T_STRING){
                        datatype = DTstring;
                    }
                    ++dim1;
                }
            }
            if (rank == 1) dspace.getSimpleExtentDims(&dim0);
            break;
        }
        default:
            STHROW("H5 data type not implemented ");
            break;
        }
    }
    catch (Exception error){
        STHROW("Error while trying to check data type; H5 Error: " << error.getDetailMsg() );
    }

}

} // namespace end
