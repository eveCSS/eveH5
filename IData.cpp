#include <string.h>
#include <stdlib.h>
#include <set>
#include "IData.h"
#include <math.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

#define STHROW(msg) { \
     ostringstream err;\
     err<<msg; \
     throw runtime_error(err.str()); }

namespace eve {

IData::IData(IMetaData& dInfo) : IMetaData(dInfo)
{
}

// reduce or extent the data
/**
 * @brief          reduce or extent the data to the new list of posrefs
 * posrefs         list of new posrefs
 */
IData::IData(IData& data, vector<int> posrefs) : IMetaData(data)
{
    if (!isArrayData()){
        set<int> intarrs;
        set<int> dblarrs;
        set<int> strarrs;
        int pcSize = posrefs.size();

        // init
        for(auto const &vpair : data.intsptrmap) {
            int key = vpair.first;
            intarrs.insert(key);
            intsptrmap.insert(pair<int, shared_ptr<vector<int>>>(key, make_shared<vector<int>>(pcSize)));
        }
        for(auto const &vpair : data.dblsptrmap) {
            int key = vpair.first;
            dblarrs.insert(key);
            dblsptrmap.insert(pair<int, shared_ptr<vector<double>>>(key, make_shared<vector<double>>(pcSize)));
        }
        for(auto const &vpair : data.strsptrmap) {
            int key = vpair.first;
            strarrs.insert(key);
            strsptrmap.insert(pair<int, shared_ptr<vector<string>>>(key, make_shared<vector<string>>(pcSize)));
        }

        unsigned int idx=0;
        unsigned int pcidx=0;
        for (int newpc : posrefs){
            while((data.posCounts[idx] < newpc) && (idx < data.posCounts.size())) ++idx;
            if (data.posCounts[idx] == newpc) {
                for (int j : intarrs){
                    intsptrmap.at(j)->at(pcidx) = data.intsptrmap.at(j)->at(idx);
                }
                for (int j : dblarrs){
                    dblsptrmap.at(j)->at(pcidx) = data.dblsptrmap.at(j)->at(idx);
                }
                for (int j : strarrs){
                    strsptrmap.at(j)->at(pcidx) = data.strsptrmap.at(j)->at(idx);
                }
                ++idx;
            }
            else if (data.posCounts[idx] != newpc) {
                for (int j : intarrs){
                    intsptrmap.at(j)->at(pcidx) = INT_MIN;
                }
                for (int j : dblarrs){
                    dblsptrmap.at(j)->at(pcidx) = NAN;
                }
                for (int j : strarrs){
                    strsptrmap.at(j)->at(pcidx) = "";
                }
            }
            if (idx >= data.posCounts.size()) --idx;
            ++pcidx;
        }
    }
    else {
        set<int> setposrefs(posrefs.begin(), posrefs.end());
        for (int pc : posCounts) {
            if ((setposrefs.find(pc) != setposrefs.end()) && (posPtrHash.find(pc) != posPtrHash.end())) posPtrHash.erase(pc);
        }
    }
    posCounts = posrefs;
}

IData::~IData()
{
}


/**
 * @brief          retrieve a pointer to stored array data
 * @param posRef   corresponding position reference
 * @param ptr      pointer to a location where the data container will be stored
 * @return         count of container members or -1 if an error occured
 */
int IData::getArrayDataPointer(int posRef, void** ptr)
{
    if (isArrayData()){
        map<int, void*>::iterator it = posPtrHash.find(posRef);
        if ( it != posPtrHash.end()) {
            *ptr = it->second;
            return dim1;
        }
    }
    return -1;
}

/**
 * @brief       retrieve a pointer to stored data
 * @param ptr   pointer to a location where the data container will be stored
 * @param col   column, if multicolumn data
 * @return      count of container members or -1 if an error occured
 */
int IData::getDataPointer(void** ptr, int col)
{
    *ptr = NULL;
    if (!isArrayData() && (col >= 0)) {
        if ((datatype == DTint32) && (intsptrmap.find(col) != intsptrmap.end())) {
                *ptr = intsptrmap.at(col).get();
                return intsptrmap.at(col)->size();
        }
        else if ((datatype == DTfloat64) && (dblsptrmap.find(col) != dblsptrmap.end())) {
                *ptr = dblsptrmap.at(col).get();
                return dblsptrmap.at(col)->size();
        }
        else if ((datatype == DTstring) && (strsptrmap.find(col) == strsptrmap.end())){
                *ptr = strsptrmap.at(col).get();
                return strsptrmap.at(col)->size();
        }
    }
    return -1;
}

vector<int> IData::getAverageAttemptsPreset(){
    if (intsptrmap.find(AVATTPR) != intsptrmap.end()) return *intsptrmap.at(AVATTPR);
    return vector<int>();
}
vector<int> IData::getAverageAttempts(){
    if (intsptrmap.find(AVATT) != intsptrmap.end()) return *intsptrmap.at(AVATT);
    return vector<int>();
}
vector<int>  IData::getAverageCountPreset(){
    if (intsptrmap.find(AVCOUNTPR) != intsptrmap.end()) return *intsptrmap.at(AVCOUNTPR);
    return vector<int>();
}
vector<int>  IData::getAverageCount(){
    if (intsptrmap.find(AVCOUNT) != intsptrmap.end()) return *intsptrmap.at(AVCOUNT) ;
    return vector<int>();
}
vector<double>  IData::getAverageLimitPreset(){
    if (dblsptrmap.find(AVLIMIT) != dblsptrmap.end()) return *dblsptrmap.at(AVLIMIT);
    return vector<double>();
}
vector<double>  IData::getAverageMaxDeviationPreset(){
    if (dblsptrmap.find(AVMAXDEV) != dblsptrmap.end()) return *dblsptrmap.at(AVMAXDEV);
    return vector<double>();
}
vector<int>  IData::getStddevCount(){
    if (intsptrmap.find(STDDEVCOUNT) != intsptrmap.end()) return *intsptrmap.at(STDDEVCOUNT);
    return vector<int>();
}
vector<double>  IData::getStddeviation(){
    if (dblsptrmap.find(STDDEV) != dblsptrmap.end()) return *dblsptrmap.at(STDDEV);
    return vector<double>();
}
vector<double>  IData::getTriggerIntv(){
    if (dblsptrmap.find(TRIGGERINTV) != dblsptrmap.end()) return *dblsptrmap.at(TRIGGERINTV);
    return vector<double>();
}

} // namespace end
