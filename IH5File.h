#ifndef IH5FILE_H
#define IH5FILE_H

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <iterator>
#include <memory>
#include "eve.h"
#include "H5Cpp.h"
#include "IData.h"
#include "IMetaData.h"

#ifndef H5_NO_NAMESPACE
     using namespace H5;
#endif

using namespace std;

namespace eve {

class IH5File {
public:
    IH5File(H5::H5File, string);
    virtual ~IH5File();
    virtual void init();
//    void close();

    virtual string getSectionString(Section);
    virtual vector<int> getChains(){return chainList;};
    virtual int getChain(){return selectedChain;};
    virtual void setChain(int chain);
    virtual map<string, string>& getChainMetaData(){return chainAttributes;};
    virtual map<string, string>& getFileMetaData(){return rootAttributes;};
    virtual void chainInventory();
    virtual vector<MetaData *> getMetaData(Section section, string str);
    virtual vector<Data*> getData(vector<MetaData*>& mdvec);
    virtual std::vector<Data*> getJoinedData(vector<MetaData*>& mdvec, FillRule fill=NoFill);
    virtual std::vector<Data*> getPreferredData(FillRule fill=NoFill);
    virtual vector<string> getLogData();

protected:
    virtual Data* getData(MetaData* );
    string filename;
    float h5version;
    int selectedChain;
    bool isOpen;
    void readDataArray(IData* data);
    void readDataPCOneCol(IData* data);
    void readDataPCTwoCol(IData* data);
    void copyAndFill(IData *srcdata,eve::DataType srctype, int srccol, IData *dstdata, eve::DataType dsttype, int dstcol);
    virtual void addExtensionData(IData* data);
    void openGroup(Group& h5group, string path);
    void closeGroup(Group& h5group);
    virtual bool isChainSection(string);
    virtual bool isCalc(string);
    virtual bool isNormalization(string);
    vector<MetaData *> getMetaData(vector<IMetaData *> *devlist, string path, string filter);
    virtual MetaData* findMetaData(vector<IMetaData *> &mdlist, string);
    virtual vector<string> getGroups(Group& group);
    virtual vector<int> getNumberGroups(Group& group);
    virtual void parseDatasets(Group& group, string prefix, vector<IMetaData*>& imeta, string calctype);
    virtual void parseGroupDatasets(Group& group, string prefix, vector<IMetaData*>& imeta, string calctype);
    map<string, string> getH5Attributes(H5Location&);
    bool haveGroupWithName(Group& group, string name);
    H5File h5file;
    IMetaData* timestampMeta;
    string chainTSfullname;
    set<string> sections;
    set<string> calculations;
    set<string> normalizations;
    vector<IMetaData*> chainmeta;
    vector<IMetaData*> extensionmeta;
    vector<IMetaData*> monitormeta;
    vector<int> chainList;
    map<string, string> rootAttributes;
    map<string, string> chainAttributes;
};

} // namespace end

#endif // IH5FILE_H
