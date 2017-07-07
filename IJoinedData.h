#ifndef IEVEJOINDATA_H
#define IEVEJOINDATA_H

#include <vector>
#include "IData.h"
#include <set>

using namespace std;

namespace eve {

class IJoinedData : public JoinedData
{
public:
    IJoinedData(vector<IData*>, FillRule);
    virtual ~IJoinedData();
    string getColumnId(unsigned int);
    IData* getData(unsigned int column);
    unsigned int getColumnCount(){return dataList.size();};
    unsigned int getValueCount(){return posCounters.size();};
    IMetaData* getMetaData(unsigned int column);


private:
    vector<IData*> dataList;
    vector<IData*> modDataList;
    vector<int> posCounters;

};
} // namespace end

#endif // IEVEJOINDATA_H
