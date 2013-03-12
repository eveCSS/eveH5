
#include <stdlib.h>
#include "IEveDataInfo.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#define STHROW(msg) { \
     ostringstream err;\
     err<<msg; \
     throw runtime_error(err.str()); }

IEveDataInfo::IEveDataInfo() : calculation(EVEraw), datatype(DTunknown), devtype(DEVTUnknown),
    dstype(EVEDSTUnknown)
{
    dim0 = 0;
    dim1 = 0;
    h5dimensions[0]=0;
    h5dimensions[1]=0;
}

void IEveDataInfo::setDataType(DataSet& ds){

    dstype = EVEDSTUnknown;
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
            datatype = DTint32;
            break;
        case H5T_FLOAT:
            dstype = EVEDSTArray;
            datatype = DTfloat64;
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
                DataType memberDtyp = compdt.getMemberDataType(idx);
                int H5class = memberDtyp.getClass();
                if ((idx == 0) && (compdt.getMemberName(idx).compare("PosCounter") == 0)){
                    if (count == 2) {
                        dstype = EVEDSTPCOneColumn;
                    }
                    else if (count == 3) {
                        dstype = EVEDSTPCTwoColumn;
                    }
                }
                else {
                    if (H5class == H5T_INTEGER)
                        datatype = DTint32;
                    else if (H5class == H5T_FLOAT)
                        datatype = DTfloat64;
                    else if (H5class == H5T_STRING)
                        datatype = DTstring;

                    ++dim1;
                }
            }
            if (rank == 1) dspace.getSimpleExtentDims(&dim0);
            break;
        }
        default:
            STHROW("DataType unimplemented ");
            break;
        }
    }
    catch (Exception error){
        STHROW("Error while trying to check data type; H5 Error: " << error.getDetailMsg() );
    }

}

string IEveDataInfo::toCalcString(EVECalc calc){
    switch (calc) {
    case EVEraw:
        return "raw";
    case EVEcenter:
        return "center";
    case EVEedge:
        return "edge";
    case EVEfwhm:
        return "fwhm";
    case EVEmaximum:
        return "max";
    case EVEminimum:
        return "min";
    case EVEpeak:
        return "peak";
    case EVEmean:
        return "mean";
    case EVEnormalized:
        return "normalized";
    case EVEstddev:
        return "stddev";
    case EVEsum:
        return "sum";
    case EVEmeta:
        return "meta";
    default:
        return "unknown";
    }
}

EVECalc IEveDataInfo::toCalcType(string calc){
    if (calc.compare("raw") == 0)
        return EVEraw;
    else if (calc.compare("center") == 0)
        return EVEcenter;
    else if (calc.compare("edge") == 0)
        return EVEedge;
    else if (calc.compare("fwhm") == 0)
        return EVEfwhm;
    else if (calc.compare("max") == 0)
        return EVEmaximum;
    else if (calc.compare("min") == 0)
        return EVEminimum;
    else if (calc.compare("peak") == 0)
        return EVEpeak;
    else if (calc.compare("mean") == 0)
        return EVEmean;
    else if (calc.compare("normalized") == 0)
        return EVEnormalized;
    else if (calc.compare("stddev") == 0)
        return EVEstddev;
    else if (calc.compare("sum") == 0)
        return EVEsum;
    else if (calc.compare("meta") == 0)
        return EVEmeta;
    return EVEunknown;
}
