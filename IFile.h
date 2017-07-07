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
#include "IJoinedData.h"

#ifndef H5_NO_NAMESPACE
     using namespace H5;
#endif

using namespace std;

namespace eve {

class IFile : public DataFile {
public:
    IFile(string);
    virtual ~IFile();
    void close();

    vector<int> getChains(){return ih5file->getChains();};
    int getChain(){return ih5file->getChain();};
    void setChain(int chain){ih5file->setChain(chain);};
    map<string, string> getChainMetaData(){return ih5file->getChainMetaData();};
    map<string, string> getFileMetaData(){return ih5file->getFileMetaData();};
    vector<MetaData *> getMetaData(Section section, string str){return ih5file->getMetaData(section, str);};
    vector<Data*> getData(vector<MetaData*>& mdvec){return ih5file->getData(mdvec);};
    JoinedData* getJoinedData(vector<MetaData*>& mdvec, FillRule fill=NoFill){return ih5file->getJoinedData(mdvec, fill);};
    JoinedData* getPreferredData(FillRule fill){return ih5file->getPreferredData(fill);};
    vector<string> getLogData(){return ih5file->getLogData();};

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
