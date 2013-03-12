#include <math.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <climits>
#include "IEveJoinData.h"
#include <set>

#define STHROW(msg) { \
     std::ostringstream err;\
     err<<msg; \
     throw std::runtime_error(err.str()); }

EveJoinData* EveJoinData::getCombinedData(vector<EveData*>* datalist, EVEFillType fill){

//   we need to build a new list to do proper casting
//   reinterpret_cast<vector<IEveData*>* >(datalist) won't work
    vector<IEveData*>* newList = new vector<IEveData*>();
    for (vector<EveData*>::iterator it = datalist->begin(); it != datalist->end(); ++it)
        newList->push_back(static_cast<IEveData*>(*it));
    IEveJoinData* joinData = new IEveJoinData(newList, fill);
    delete newList;
    return joinData;
}


IEveJoinData::IEveJoinData(vector<IEveData*>* dataList, EVEFillType fillType)
{

    valueCount = 0;
    colCount = 0;
    set<int> posCounts;
    vector<dataEntry *> columnList;

    // fill an ordered unique set with all posCounts, create container
    for (vector<IEveData*>::iterator it=dataList->begin(); it != dataList->end(); ++it){
        if(!(*it)->isArrayData()) {
            dataEntry* dEntry = new dataEntry;
            dEntry->posRefs = (*it)->getPosReferences();
            posCounts.insert(dEntry->posRefs.begin(), dEntry->posRefs.end());
            dEntry->offSet = 0;
            dEntry->lastPosRef=0;
            dEntry->deviceType = (*it)->getDeviceType();
            dEntry->colNumber = colCount;
            dEntry->dataPtr = *it;
            columnList.push_back(dEntry);
            colIds.push_back((*it)->getId());
            EVEDataType dt = (*it)->getDataType();
            colTypes.push_back(dt);
            if (dt == DTint32)
                dataPtrs.push_back(new vector<int>());
            else if (dt == DTfloat64)
                dataPtrs.push_back(new vector<double>());
            else if (dt == DTstring)
                dataPtrs.push_back(new vector<string>());
            else
                STHROW("unsupported datatype, currently only int32, float64 and strings are supported");
            ++colCount;
        }
    }

    for (set<int>::iterator it=posCounts.begin(); it!=posCounts.end(); ++it){
        int currentPos = *it;

        bool haveAxis = false;
        bool haveChannel = false;
        bool skipCurrent = false;
        for (vector<dataEntry*>::iterator dEntrit=columnList.begin(); dEntrit != columnList.end(); ++dEntrit){
            if (findInt((*dEntrit)->posRefs, currentPos)){
                // column has this posCounter
                (*dEntrit)->lastPosRef=currentPos;
                fillIn(*dEntrit, true);
                if ((*dEntrit)->deviceType == DEVTAxis) haveAxis = true;
                if ((*dEntrit)->deviceType == DEVTChannel) haveChannel = true;
            }
            else if (((*dEntrit)->deviceType == DEVTAxis)  && (*dEntrit)->lastPosRef
                     && ((*dEntrit)->lastPosRef < currentPos)
                     && ((fillType == LastFill)||(fillType == LastNANFill))){
                // use this posCount with lastAxis value
                fillIn(*dEntrit, true);
            }
            else if (((*dEntrit)->deviceType == DEVTChannel)
                     && ((fillType == NANFill)||(fillType == LastNANFill))){
                // use this posCount with Nan
                fillIn(*dEntrit, false);
            }
            else {
                // skip this datapoint
                skipCurrent = true;
                break;
            }
        }
        if (skipCurrent){
            rollBack();
        }
        else if (haveAxis && haveChannel){
            posCounters.push_back(currentPos);
            ++valueCount;
        }
        else {
            rollBack();
        }
    }
}

IEveJoinData::~IEveJoinData(){
    //TODO
}

bool IEveJoinData::findInt(vector<int>& haystack, int needle){
    for (vector<int>::iterator it=haystack.begin(); (it != haystack.end() && ((*it) <= needle)); ++it)
        if ((*it) == needle) return true;

    return false;
}

void IEveJoinData::fillIn(dataEntry* data, bool usePosRef){

    if (usePosRef && (data->posRefs[data->offSet] != data->lastPosRef))
        for (unsigned int i=data->offSet; i < data->posRefs.size(); ++i)
            if (data->posRefs[i] == data->lastPosRef) {
                data->offSet=i;
                break;
            }

     switch (colTypes[data->colNumber]){
     case DTint32:
         if (usePosRef)
             ((vector<int>*)dataPtrs[data->colNumber])->push_back(data->dataPtr->intvect[data->offSet]);
         else
             ((vector<int>*)dataPtrs[data->colNumber])->push_back(INT_MIN);
         break;

     case DTfloat64:
         if (usePosRef)
             ((vector<double>*)dataPtrs[data->colNumber])->push_back(data->dataPtr->dblvect[data->offSet]);
         else
             ((vector<double>*)dataPtrs[data->colNumber])->push_back(NAN);
         break;
     case DTstring:
         if (usePosRef)
             ((vector<string>*)dataPtrs[data->colNumber])->push_back(data->dataPtr->strvect[data->offSet]);
         else
             ((vector<string>*)dataPtrs[data->colNumber])->push_back(string());
         break;
     default:
         STHROW("unsupported datatype, currently only int32, float64 and strings are supported");
     }
}

void IEveJoinData::rollBack(){
    for (unsigned int i=0; i<colCount; ++i){
        if (colTypes[i] == DTint32){
            while (((vector<int>*)dataPtrs[i])->size() > valueCount) ((vector<int>*)dataPtrs[i])->pop_back();
        }
        else if (colTypes[i] == DTfloat64) {
            while (((vector<double>*)dataPtrs[i])->size() > valueCount) ((vector<double>*)dataPtrs[i])->pop_back();
        }
        else if (colTypes[i] == DTstring) {
            while (((vector<string>*)dataPtrs[i])->size() > valueCount) ((vector<string>*)dataPtrs[i])->pop_back();
        }
        else
            STHROW("unsupported datatype, currently only int32, float64 and strings are supported");
    }
}


string IEveJoinData::getColumnId(unsigned int column)
{
    if (column < colIds.size())
        return colIds[column];
    else
        STHROW("Column number out of bounds: "<<column);
    return "";
}

int IEveJoinData::getColumnPointer(unsigned int column, void** ptr)
{
    if (column < colCount) {
            *ptr = dataPtrs[column];
            return valueCount;
    }
    else
        STHROW("Column number out of bounds: "<<column);
    return 0;
}

EVEDataType IEveJoinData::getColumnType(unsigned int column)
{
    if (column < colTypes.size())
        return colTypes[column];
    else
        STHROW("Column number out of bounds: "<<column);
    return DTunknown;
}
