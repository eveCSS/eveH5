#include <string.h>
#include <stdlib.h>
#include "IEveData.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#define STHROW(msg) { \
     ostringstream err;\
     err<<msg; \
     throw runtime_error(err.str()); }


IEveData::IEveData(IEveDataInfo& dInfo) : IEveDataInfo(dInfo)
{

}

IEveData::IEveData(IEveData& dInfo) : IEveDataInfo(dInfo), posPtrHash(dInfo.posPtrHash),
    intvect(dInfo.intvect), dblvect(dInfo.dblvect), strvect(dInfo.strvect)
{
    int count = 2;
    int element_size = 0;
    if ((dstype == EVEDSTPCTwoColumn) || (dstype == EVEDSTArray)) {
        if (dstype == EVEDSTArray) count = h5dimensions[0];
        if (datatype == DTfloat64)
            element_size = sizeof(double);
        else if (datatype == DTint32)
            element_size = sizeof(int);
        else
            STHROW("unsupported datatype, currently only int32 or float64 are supported for arrays");

        for (map<int, void*>::iterator it=posPtrHash.begin(); it!=posPtrHash.end(); ++it){
            void* memBuffer = malloc(element_size * count);
            if (memBuffer == NULL)
                STHROW("Unable to allocate memory ");
            it->second = memcpy(memBuffer, it->second, element_size * count);
        }
    }
}

IEveData::~IEveData()
{
    for (map<int, void*>::iterator it=posPtrHash.begin(); it!=posPtrHash.end(); ++it){
        void * ptr = it->second;
        if (ptr != NULL) free(ptr);
    }
}

/**
 * @brief          check if stored data is array data
 * @return         true if is it array data, else false
 */
bool IEveData::isArrayData(){
    if ((dstype == EVEDSTPCTwoColumn) || (dstype == EVEDSTArray)) return true;

    return false;
}

/**
 * @brief          retrieve a pointer to stored array data
 * @param posRef   corresponding position reference
 * @param ptr      pointer to a location where the data container will be stored
 * @return         count of container members or -1 if an error occured
 */
int IEveData::getArrayDataPointer(int posRef, void** ptr)
{
    if (isArrayData()){
        map<int, void*>::iterator it = posPtrHash.find(posRef);
        if ( it != posPtrHash.end()) {
            *ptr = it->second;
            return dim1;
        }
    }
    return -1;
}

/**
 * @brief       retrieve a pointer to stored data
 * @param ptr   pointer to a location where the data container will be stored
 * @return      count of container members or -1 if an error occured
 */
int IEveData::getDataPointer(void** ptr)
{
    if (!isArrayData()) {
        if (datatype == DTint32) {
            *ptr = &intvect;
            return intvect.size();
        }
        else if (datatype == DTfloat64) {
            *ptr = &dblvect;
            return dblvect.size();
        }
        else if (datatype == DTstring) {
            *ptr = &strvect;
            return strvect.size();
        }
    }
    return -1;
}
