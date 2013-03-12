#ifndef EVEH5FILE_H
#define EVEH5FILE_H

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <iterator>
#include "eveH5.h"
#include "H5Cpp.h"
#include "IEveData.h"
#include "IEveDataInfo.h"

#ifndef H5_NO_NAMESPACE
     using namespace H5;
#endif

using namespace std;

class eveData;

class IEveH5File : public EveH5File {
public:
    IEveH5File(string);
    virtual ~IEveH5File();
    void close();

    vector<string> getChainNames(){return chainList;};
    vector<string> getDeviceNames();
    vector<string> getDeviceIds();
    vector<string> getDeviceIdForName(string);
    vector<string> getCalcNames(string);

    IEveDataInfo* getDataInfo(string chain, string xmlid);
    IEveDataInfo* getDataInfo(string chain, string axisId, string channelId, string normalizeId, string calculation);
    IEveDataInfo* getDataInfoByName(string chain, string name);
    IEveDataInfo* getDataInfoByName(string chain, string axisId, string channelId, string normalizeId, string calculation);

    IEveData* getData(EveDataInfo*);

    IEveDataInfo* getDeviceInfo(string);
    IEveDataInfo* getDeviceInfoByName(string);
    virtual multimap<string, string>& getRootAttributes(){return rootAttributes;};

private:
    void open(string);
    bool isOpen;
    bool haveCalculation(string, string);
    bool haveDeviceGroup;
    IEveData* getDataSkipVerify(IEveDataInfo*);
    list<string> getDatasetFullIds(){return dsList;};
    list<string> getDatasetIds(string, EVECalc modified=EVEraw);
    int getAttributes(H5Object&, multimap<string, string>&);
    void getRootGroups(Group&);
    void parseDatasets(Group&, string, EVECalc, list<string>&);
    void parseGroups(Group&, string);
    H5File h5file;
    multimap<string, string> name2xmlid;
    map<string, IEveDataInfo*> xmlidMap; // fulldsname
    multimap<string, string> chain2Modified;
    vector<string> chainList;
    list<string> dsList;
    list<string> modDsList;
    list<string> deviceDsList;
    multimap<string, string> rootAttributes;
};
#endif // EVEH5FILE_H
