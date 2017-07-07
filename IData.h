#ifndef EVEDATA_H
#define EVEDATA_H

#include <iostream>
#include <vector>
#include <memory>
#include <map>
#include "eve.h"
#include "IMetaData.h"

using namespace std;

namespace eve {

#define INTVECT1 0
#define INTVECT2 1
#define AVATTPR 2
#define AVATT 3
#define AVCOUNT 4
#define AVCOUNTPR 5
#define STDDEVCOUNT 6
#define INTVECTMAX 7

#define DBLVECT1 0
#define DBLVECT2 1
#define AVLIMIT 2
#define AVMAXDEV 3
#define STDDEV 4
#define TRIGGERINTV 5
#define DBLVECTMAX 6

#define STRVECT1 0
#define STRVECT2 1
#define STRVECTMAX 2


class IData : public IMetaData, public Data
{

public:
    IData(IMetaData&);
    IData(IData&, vector<int>);
    virtual ~IData();

    string getName(){return IMetaData::getName();};
    string getUnit(){return IMetaData::getUnit();};
    string getId(){return IMetaData::getId();};
    string getChannelId(){return IMetaData::getChannelId();};
    string getNormalizeId(){return IMetaData::getNormalizeId();};
    pair<int, int> getDimension(){return IMetaData::getDimension();};
    Section getSection(){return IMetaData::getSection();};
    map<std::string, std::string>& getAttributes(){return IMetaData::getAttributes();};
    DeviceType getDeviceType(){return IMetaData::getDeviceType();};
    eve::DataType getDataType(){return IMetaData::getDataType();};

    vector<int> getPosReferences(){return posCounts;};
    int getArrayDataPointer(int posRef, void**);
    int getDataPointer(void** ptr){return getDataPointer(ptr, 0);};
    int getDataPointer(void** ptr, int col);
    bool isArrayData(){if (dstype == EVEDSTArray) return true; else return false;};
    bool hasAverageData(){if ((intsptrmap.find(AVCOUNT) != intsptrmap.end()) && (intsptrmap.at(AVCOUNT)->size() > 0)) return true; else return false; };
    bool hasStdDeviation(){if ((dblsptrmap.find(STDDEV) != dblsptrmap.end()) && (dblsptrmap.at(STDDEV)->size() > 0)) return true; else return false; };
    vector<int> getAverageAttemptsPreset();
    vector<int> getAverageAttempts();
    vector<int> getAverageCountPreset();
    vector<int> getAverageCount();
    vector<double> getAverageLimitPreset();
    vector<double> getAverageMaxDeviationPreset();
    vector<int> getStddevCount();
    vector<double> getStddeviation();
    vector<double> getTriggerIntv();


private:
    vector<int> posCounts;
    map<int, void*> posPtrHash;
    map<int, shared_ptr<vector<int>>> intsptrmap;
    map<int, shared_ptr<vector<double>>> dblsptrmap;
    map<int, shared_ptr<vector<string>>> strsptrmap;

    friend class IH5File;
    friend class IH5FileV5;
    friend class IJoinedData;
};

} // namespace end

#endif // EVEDATA_H
