#ifndef EVEDATAINFO_H
#define EVEDATAINFO_H

#include <iostream>
#include <vector>
#include <map>
#include "eveH5.h"
#include "H5Cpp.h"

#ifndef H5_NO_NAMESPACE
     using namespace H5;
#endif
using namespace std;

enum EVECalc { EVEunknown, EVEraw, EVEcenter, EVEedge, EVEfwhm, EVEmaximum, EVEminimum, EVEpeak, EVEmean, EVEnormalized, EVEstddev, EVEsum, EVEmeta } ;
enum EVEDatasetType { EVEDSTUnknown, EVEDSTPCOneColumn, EVEDSTPCTwoColumn, EVEDSTArray};

class IEveDataInfo : public EveDataInfo
{
public:
    IEveDataInfo();
    virtual ~IEveDataInfo(){};
    string getId(){return xmlId;};
    string getChannelId(){return channelId;};
    string getNormalizeId(){return normalizeId;};
    string getCalculation(){return toCalcString(calculation);};
    std::pair<int, int> getDimension(){return std::pair<int, int>(dim0, dim1);};
    EVEDeviceType getDeviceType(){return devtype;};
    EVEDataType getDataType(){return datatype;};
    virtual multimap<string, string>& getAttributes(){return attributes;};
    static string toCalcString(EVECalc);
    static EVECalc toCalcType(string);

protected:
    void setDataType(DataSet& ds);
    multimap<string, string> attributes;
    string path;
    string h5name;
    string name;
    string xmlId;
    string channelId;
    string normalizeId;
    EVECalc calculation;
    EVEDataType datatype;
    EVEDeviceType devtype;
    EVEDatasetType dstype;
    vector<int> posCounts;
    hsize_t dim0; // Anzahl der PosCounts
    hsize_t dim1; // > 1 bei arrayData und EVEDSTPCTwoColumn
    hsize_t h5dimensions[2];
    friend class IEveH5File;
};

#endif // EVEDATAINFO_H
