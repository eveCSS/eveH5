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
    vector<string> getMonitorDeviceIds();
    vector<string> getPathsForId(string dataId);
    IEveDataInfo* getDataInfoForPath(string path);
    IEveDataInfo* getMonitorDataInfo(string);
//    vector<string> getCalcNames(string);
//    IEveDataInfo* getDataInfo(string chain, string xmlid);
//    IEveDataInfo* getDataInfo(string chain, string axisId, string channelId, string normalizeId, string calculation);
//    IEveDataInfo* getDataInfoByName(string chain, string name);
//    IEveDataInfo* getDataInfoByName(string chain, string axisId, string channelId, string normalizeId, string calculation);
    IEveData* getData(EveDataInfo*);
    virtual multimap<string, string> getRootAttributes(){return rootAttributes;};
    virtual multimap<string, string> getChainAttributes(string);

private:
    void open(string);
    bool isOpen;
//    bool haveCalculation(string, string);
    void checkVersion(Group&);
    IEveData* getDataSkipVerify(IEveDataInfo*);
    // list<string> getDatasetFullIds(){return dsList;};
    // list<string> getDatasetIds(string, EVECalc modified=EVEraw);
    vector<string> getGroups(Group& group, string);
    void openGroup(Group& h5group, string path);
    void closeGroup(Group& h5group);
    int getAttributes(H5Location&, multimap<string, string>&);
    void parseDatasets(Group&, string, map<string, IEveDataInfo *> *idmap, multimap<string, string> *nameidmap);
    void parseGroupDatasets(Group& group, string prefix, map<string, IEveDataInfo*> *idmap, multimap<string, string> *nameidmap);
    bool haveGroupWithName(Group& group, string name);
    H5File h5file;
    multimap<string, string> name2xmlid;
    map<string, IEveDataInfo*> xmlidMap; // fulldsname
    map<string, IEveDataInfo*> monitorxmlidMap; // fulldsname
//    multimap<string, string> chain2Modified;
    vector<string> chainList;
//    list<string> dsList;
//    list<string> deviceDsList;
    multimap<string, string> rootAttributes;
};
#endif // EVEH5FILE_H
