#ifndef IFILE_H
#define IFILE_H

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <iterator>
#include "eve.h"
#include "H5Cpp.h"
#include "IData.h"
#include "IH5File.h"
#include "IMetaData.h"

#ifndef H5_NO_NAMESPACE
     using namespace H5;
#endif

using namespace std;

namespace eve {

class IFile : public DataFile {
public:
    IFile(string);
    virtual ~IFile();

    vector<int> getChains(){return ih5file->getChains();};
    int getChain(){return ih5file->getChain();};
    void setChain(int chain){ih5file->setChain(chain);};
    ChainMetaData* getChainMetaData(){return ih5file->getChainMetaData();};
    FileMetaData* getFileMetaData(){return ih5file->getFileMetaData();};
    vector<MetaData *> getMetaData(Section section, string id, string name){return ih5file->getMetaData(section, id, name);};
    vector<Data*> getData(vector<MetaData*>& mdvec){return ih5file->getData(mdvec);};
    vector<Data*> getJoinedData(vector<MetaData*>& mdvec, FillRule fill=NoFill){return ih5file->getJoinedData(mdvec, fill);};
    vector<Data*> getPreferredData(FillRule fill){return ih5file->getPreferredData(fill);};
    vector<string> getLogData(){return ih5file->getLogData();};
    string getNameById(Section section, std::string id){return ih5file->getNameById(section, id);};

private:
    IH5File* ih5file;
    float h5version;
    void openGroupH5(H5File, Group&, string);
    void closeGroupH5(Group&);
    void getH5Version(Group&);
    void close(H5File);

};

} // namespace end

#endif // IFILE_H
