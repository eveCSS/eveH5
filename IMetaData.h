#ifndef EVEMETADATA_H
#define EVEMETADATA_H

#include <iostream>
#include <vector>
#include <map>
#include "eve.h"
#include "H5Cpp.h"

#ifndef H5_NO_NAMESPACE
     using namespace H5;
#endif
using namespace std;

namespace eve {

enum EVEDatasetType { EVEDSTUnknown, EVEDSTPCOneColumn, EVEDSTPCTwoColumn, EVEDSTArray};

class IMetaData : public MetaData
{
public:
    IMetaData();
    IMetaData(string , string , string , Section section, map<string, string> );
    virtual ~IMetaData(){};
    virtual string getName(){return name;};
    virtual string getUnit();
    virtual string getId(){return xmlId;};
    virtual string getChannelId(){return channelId;};
    virtual string getNormalizeId(){return normalizeId;};
    virtual pair<unsigned int, unsigned int> getDimension(){return pair<unsigned int, unsigned int>(dim0, dim1);};
    virtual Section getSection(){return selSection;};
    virtual map<string, string>& getAttributes(){return attributes;};
    virtual DeviceType getDeviceType(){return devtype;};
    virtual eve::DataType getDataType(){return datatype;};

protected:
    virtual string getPath(){return path;};
    virtual string getH5name(){return h5name;};
    virtual string getFQH5Name();
    void setDataType(DataSet& ds);
    Section selSection;
    string path;
    string calculation;
    string h5name;
    map<string, string> attributes;
    string name;
    string xmlId;
    string channelId;
    string normalizeId;
    eve::DataType datatype;
    DeviceType devtype;
    EVEDatasetType dstype;
    hsize_t dim0; // Anzahl der PosCounts
    hsize_t dim1; // > 1 bei arrayData und EVEDSTPCTwoColumn
    hsize_t h5dimensions[2];

    friend class IH5File;
    friend class IH5FileV5;
};
} // namespace end

#endif // EVEMETADATA_H
