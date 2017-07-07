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

JoinedData* JoinedData::JoinedData(vector<Data*>* datalist, FillRule fill){

//   we need to build a new list to do proper casting
//   reinterpret_cast<vector<IData*>* >(datalist) won't work
    vector<IData*> newList;
    for (vector<Data*>::iterator it = datalist->begin(); it != datalist->end(); ++it)
        newList.push_back(static_cast<IData*>(*it));
    JoinedData* joinData = new IJoinedData(newList, fill);
    return joinData;
}

IJoinedData::IJoinedData(vector<IData*> datalist, FillRule fillType)
{


    // 1. Es wird erstmal nur ein Set mit den pos-refs erzeugt, die im joined Data vorhanden sein müssen.
    //    Dabei wird nach dev-type unterschieden und LastFill oder NanFill beachtet.
    // 2. Es wird eine Kopie der datalist => joinedDatalist erstellt, die neue Einträge bekommt, wenn auf die Original-Daten
    //    ein Fill angewendet werden muss.
    // 3. Wird getColumnPointer (keine Array Daten) aufgerufen, wird geprüft, ob die posRefs übereinstimmen, falls nicht,
    //    dann wird ein neues IData-Objekt erzeugt, mit dem richtigen LastFill bzw. Nan-Fill gemäß der joined pos-ref-Liste.
    //    Auch die average/standarddev arrays werden bearbeitet.
    //    Das neue IData-Objekt kommt in die joined datalist und es wird vermerkt, dass es gecached ist.
    // 4. Es könnte eine addData() Funktion geben, die es erlaubt ein weiteres Idata hinzuzufügen.
    //    Kann sein, dass das genauso aufwändig wird, wie die Erzeugung eines neuen JoinedData, dann weg lassen.
    // 5. Evtl. sollte es noch eine Funktion vector<IData*> getAllData() geben, die die iDataListe aus em Konstruktor zurückgibt.
    //    Damit kann man dann einen neuen modifizierten JoinedData erzeugen.


    set<int> channelPosCounts;
    set<int> axisPosCounts;
    set<int> newlist;

    dataList = datalist;    // list of original data objects
    modDataList = datalist; // list of data objects which may be modified

    // fill an ordered unique set with all posCounters, create container
    for (vector<IData*>::iterator it=datalist.begin(); it != datalist.end(); ++it){
        if (*it == NULL) continue;
        // TODO remove Null data objects from dataList and modDataList
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
