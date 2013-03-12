#ifndef EVEDATA_H
#define EVEDATA_H

#include <iostream>
#include <vector>
#include <map>
#include "eveH5.h"
#include "IEveDataInfo.h"

using namespace std;

class IEveData : public IEveDataInfo, public EveData
{
    // alle Daten (bei MCAs sind das viele) werden
    // auf einmal gelesen.

public:
    IEveData(IEveDataInfo&);
    IEveData(IEveData&);
    virtual ~IEveData();
    // TODO we need a copy constructor and assignment operator

    string getId(){return IEveDataInfo::getId();};
    string getChannelId(){return IEveDataInfo::getChannelId();};
    string getNormalizeId(){return IEveDataInfo::getNormalizeId();};
    string getCalculation(){return IEveDataInfo::getCalculation();};
    pair<int, int> getDimension(){return IEveDataInfo::getDimension();};
    multimap<std::string, std::string>& getAttributes(){return IEveDataInfo::getAttributes();};
    EVEDeviceType getDeviceType(){return IEveDataInfo::getDeviceType();};
    EVEDataType getDataType(){return IEveDataInfo::getDataType();};

    vector<int> getPosReferences(){return posCounts;};
    int getArrayDataPointer(int posRef, void**);
    int getDataPointer(void**);
    bool isArrayData();

private:
    // for EVEDSTArray and EVEDSTPCTwoColumn
    map<int, void*> posPtrHash;
    // for EVEDSTPCOneColumn
    vector<int> intvect;
    vector<double> dblvect;
    vector<string> strvect;

    friend class IEveH5File;
    friend class IEveJoinData;
};


#endif // EVEDATA_H
