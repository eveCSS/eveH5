
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
    datatype = DTunknown;
    devtype = DEVTUnknown;
    dstype = EVEDSTUnknown;
    isMonitor = false;
}

void IEveDataInfo::setDataType(DataSet& ds){

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
            STHROW("DataType unimplemented ");
            break;
        }
        if (isMonitor) dstype = EVEDSTPCOneColumn;
    }
    catch (Exception error){
        STHROW("Error while trying to check data type; H5 Error: " << error.getDetailMsg() );
    }

}
