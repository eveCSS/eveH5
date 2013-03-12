#ifndef IEVEJOINDATA_H
#define IEVEJOINDATA_H

#include <vector>
#include "eveH5.h"
#include "IEveData.h"

using namespace std;

struct dataEntry{
    int offSet;
    EVEDeviceType deviceType;
    IEveData* dataPtr;
    // TODO use dataPtr->getPosReferences()
    vector<int> posRefs;
    int lastPosRef;
    int colNumber;
};

class IEveJoinData : public EveJoinData
{
public:
    IEveJoinData(std::vector<IEveData*>*, EVEFillType);
    virtual ~IEveJoinData();
    string getColumnId(unsigned int);
    int getColumnPointer(unsigned int, void**);
    unsigned int getColumnCount(){return colCount;};
    unsigned int getValueCount(){return valueCount;};
    EVEDataType getColumnType(unsigned int column);
    vector<int> getPosReferences(){return posCounters;};


private:
    void fillIn(dataEntry*, bool);
    bool findInt(vector<int>&, int);
    void rollBack();
    vector<void*> dataPtrs;
    vector<int> posCounters;
    vector<string> colIds;
    vector<EVEDataType> colTypes;
    unsigned int valueCount;
    unsigned int colCount;
};

#endif // IEVEJOINDATA_H
