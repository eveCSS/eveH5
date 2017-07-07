#include <math.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <climits>
#include "IJoinedData.h"

#define STHROW(msg) { \
     std::ostringstream err;\
     err<<msg; \
     throw std::runtime_error(err.str()); }

namespace eve {

JoinedData* JoinedData::getJoinedData(vector<Data*>& datalist, FillRule fill){

     return new IJoinedData(datalist, fill);
}

IJoinedData::IJoinedData(vector<Data*>& datalist, FillRule fillType)
{

    set<int> channelPosCounts;
    set<int> axisPosCounts;
    set<int> newlist;

    for (vector<Data*>::iterator it=datalist.begin(); it != datalist.end(); ++it){
        if (*it != NULL) dataList.push_back(static_cast<IData*>(*it));
    }
    // dataList             // list of original data objects
    modDataList = dataList; // list of data objects which may be modified and cached

    // fill an ordered unique set with all posCounters, create container
    for (vector<IData*>::iterator it=dataList.begin(); it != dataList.end(); ++it){
        DeviceType deviceType = (*it)->getDeviceType();
        if (deviceType == Channel) {
            channelPosCounts.insert((*it)->posCounts.begin(), (*it)->posCounts.end());
        }
        else if (deviceType == Axis) {
            axisPosCounts.insert((*it)->posCounts.begin(), (*it)->posCounts.end());
        }
    }

    if (fillType == NoFill) {
        for (set<int>::iterator it=axisPosCounts.begin(); it != axisPosCounts.end(); ++it){
            if (channelPosCounts.find(*it) != channelPosCounts.end()) newlist.insert(*it);
        }
    }
    if ((fillType == LastFill) || (fillType == LastNANFill))
        newlist.insert(channelPosCounts.begin(), channelPosCounts.end());
    if((fillType == NANFill) || (fillType == LastNANFill))
        newlist.insert(axisPosCounts.begin(), axisPosCounts.end());

    posCounters = vector<int>(newlist.begin(), newlist.end());
}

IJoinedData::~IJoinedData(){

}

IMetaData* IJoinedData::getMetaData(unsigned int column)
{
    if (column < modDataList.size())
        return new IMetaData(*((IMetaData*)modDataList[column]));
    else
        STHROW("Column number " << column << " out of bounds");

    return NULL;
}



string IJoinedData::getColumnId(unsigned int column)
{
    if (column < modDataList.size())
        return modDataList[column]->getId();
    else
        STHROW("Column number " << column << " out of bounds");
    return "";
}

IData* IJoinedData::getData(unsigned int column)
{
    IData* jdata = NULL;
    if (column < modDataList.size()) {
        if (modDataList[column] != dataList[column]) {
            jdata = new IData(*(modDataList[column]));
        }
        else {
            if ((modDataList[column]->posCounts.size() != posCounters.size()) || (modDataList[column]->posCounts != posCounters)) {
                modDataList[column] = new IData(*(dataList[column]), posCounters);
            }
            jdata =  new IData(*(modDataList[column]));
        }
    }
    else
        STHROW("Column number " << column << " out of bounds");

    return jdata;
}

} // namespace end
