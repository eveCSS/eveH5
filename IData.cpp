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

/**
 * @brief          reduce or extent the data to the new list of posrefs
 * posrefs         list of new posrefs
 */
IData::IData(IData& data, vector<int> posrefs, FillRule fillType, IData* snapdata) : IMetaData(data)
{
    if (!isArrayData()){
        set<int> intarrs;
        set<int> dblarrs;
        set<int> strarrs;
        int pcSize = posrefs.size();
        int lastint = INT_MIN;
        double lastdbl = NAN;
        string laststring = "NaN";

        if (((fillType == LastFill) || (fillType == LastNANFill)) && (data.getDeviceType() == Axis)){
            if ((snapdata != NULL) && (!snapdata->isArrayData())) {
                vector<int> snap_pc = snapdata->getPosReferences();
                if ((snap_pc.size() > 0) && (posrefs.size() > 0) && (posrefs[0] > snap_pc[0])){
                    int snap_idx=0;
                    for (unsigned int i=0; i < snap_pc.size(); ++i) if (posrefs[0] > snap_pc[i]) snap_idx = i;
                    if (snapdata->getDataType() == DTint32) {
                        lastint = ((vector<int>*)snapdata->getDataPointer())->at(snap_idx);
                    }
                    else if (snapdata->getDataType() == DTfloat64) {
                        lastdbl = ((vector<double>*)snapdata->getDataPointer())->at(snap_idx);
                    }
                    else if (snapdata->getDataType() == DTstring) {
                        laststring = ((vector<string>*)snapdata->getDataPointer())->at(snap_idx);
                    }
                }
            }
        }

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
            while((data.posCounts[idx] < newpc) && (idx < (data.posCounts.size()-1))) ++idx;
            if (data.posCounts[idx] == newpc) {
                for (int j : intarrs){
                    intsptrmap.at(j)->at(pcidx) = data.intsptrmap.at(j)->at(idx);
                    if (j == 0) lastint = data.intsptrmap.at(j)->at(idx);
                }
                for (int j : dblarrs){
                    dblsptrmap.at(j)->at(pcidx) = data.dblsptrmap.at(j)->at(idx);
                    if (j == 0) lastdbl = data.dblsptrmap.at(j)->at(idx);
                }
                for (int j : strarrs){
                    strsptrmap.at(j)->at(pcidx) = data.strsptrmap.at(j)->at(idx);
                    if (j == 0) laststring = data.strsptrmap.at(j)->at(idx);
                }
                ++idx;
            }
            else if (data.posCounts[idx] != newpc) {
                int intval = INT_MIN;
                double dblval = NAN;
                string strval = "NaN";
                for (int j : intarrs){
                    if((j == 0) && ((fillType == LastFill) || (fillType == LastNANFill)) && (data.getDeviceType() == Axis)) intval = lastint;
                    intsptrmap.at(j)->at(pcidx) = intval;
                }
                for (int j : dblarrs){
                    if((j == 0) && ((fillType == LastFill) || (fillType == LastNANFill)) && (data.getDeviceType() == Axis)) dblval = lastdbl;
                    dblsptrmap.at(j)->at(pcidx) = dblval;
                }
                for (int j : strarrs){
                    if((j == 0) && ((fillType == LastFill) || (fillType == LastNANFill)) && (data.getDeviceType() == Axis)) strval = laststring;
                    strsptrmap.at(j)->at(pcidx) = strval;
                }
            }
            if (idx >= data.posCounts.size()) --idx;
            ++pcidx;
        }
    }
    else {
        set<int> setposrefs(posrefs.begin(), posrefs.end());
        posPtrHash = data.posPtrHash;
        for (int pc : data.posCounts) {
            if ((setposrefs.find(pc) == setposrefs.end()) && (posPtrHash.find(pc) != posPtrHash.end())) posPtrHash.erase(pc);
        }
    }
    posCounts = posrefs;
    dim0 = posCounts.size();
}

IData::~IData()
{
}


void* IData::getArrayDataPointer(unsigned int row)
{
    void *ptr=NULL;
    if (isArrayData() && row < posCounts.size()){
        int posRef = posCounts[row];
        map<int, shared_ptr<char>>::iterator it = posPtrHash.find(posRef);
        if ( it != posPtrHash.end()) {
            if (datatype == DTint32)
                ptr = new vector<int>((int*)it->second.get(), (int*)it->second.get()+dim1);
            else if (datatype == DTfloat64)
                ptr = new vector<double>((double*)it->second.get(), (double*)it->second.get()+dim1);
            else if (datatype == DTint8)
                ptr = new vector<char>((char*)it->second.get(), (char*)it->second.get()+dim1);
            else if (datatype == DTint16)
                ptr = new vector<short>((short*)it->second.get(), (short*)it->second.get()+dim1);
            else if (datatype == DTint64)
                ptr = new vector<long long>((long long*)it->second.get(), (long long*)it->second.get()+dim1);
            else if (datatype == DTuint8)
                ptr = new vector<unsigned char>((unsigned char*)it->second.get(), (unsigned char*)it->second.get()+dim1);
            else if (datatype == DTuint16)
                ptr = new vector<unsigned short>((unsigned short*)it->second.get(), (unsigned short*)it->second.get()+dim1);
            else if (datatype == DTuint32)
                ptr = new vector<unsigned int>((unsigned int*)it->second.get(), (unsigned int*)it->second.get()+dim1);
            else if (datatype == DTuint64)
                ptr = new vector<unsigned long long>((unsigned long long*)it->second.get(), (unsigned long long*)it->second.get()+dim1);
            else if (datatype == DTfloat32)
                ptr = new vector<float>((float*)it->second.get(), (float*)it->second.get()+dim1);
        }
    }
    return ptr;
}

void* IData::getDataPointer()
{
    void *ptr = NULL;
    if (!isArrayData()) {
        if (((datatype == DTfloat64) || (datatype == DTfloat32)) && (dblsptrmap.find(0) != dblsptrmap.end())) {
            ptr = new vector<double>(*dblsptrmap.at(0));
        }
        else if ((datatype == DTstring) && (strsptrmap.find(0) != strsptrmap.end())){
            ptr = new vector<string>(*strsptrmap.at(0));
        }
        else if (intsptrmap.find(0) != intsptrmap.end()) {
            ptr = new vector<int>(*intsptrmap.at(0));
        }
    }
    return ptr;
}

vector<int> IData::getAverageAttemptsPreset(){
    if (intsptrmap.find(AVATTPR) != intsptrmap.end()) return vector<int>(*intsptrmap.at(AVATTPR));
    return vector<int>();
}
vector<int> IData::getAverageAttempts(){
    if (intsptrmap.find(AVATT) != intsptrmap.end()) return vector<int>(*intsptrmap.at(AVATT));
    return vector<int>();
}
vector<int>  IData::getAverageCountPreset(){
    if (intsptrmap.find(AVCOUNTPR) != intsptrmap.end()) return vector<int>(*intsptrmap.at(AVCOUNTPR));
    return vector<int>();
}
vector<int>  IData::getAverageCount(){
    if (intsptrmap.find(AVCOUNT) != intsptrmap.end()) return vector<int>(*intsptrmap.at(AVCOUNT)) ;
    return vector<int>();
}
vector<double>  IData::getAverageLimitPreset(){
    if (dblsptrmap.find(AVLIMIT) != dblsptrmap.end()) return vector<double>(*dblsptrmap.at(AVLIMIT));
    return vector<double>();
}
vector<double>  IData::getAverageMaxDeviationPreset(){
    if (dblsptrmap.find(AVMAXDEV) != dblsptrmap.end()) return vector<double>(*dblsptrmap.at(AVMAXDEV));
    return vector<double>();
}
vector<int>  IData::getStddevCount(){
    if (intsptrmap.find(STDDEVCOUNT) != intsptrmap.end()) return vector<int>(*intsptrmap.at(STDDEVCOUNT));
    return vector<int>();
}
vector<double>  IData::getStddeviation(){
    if (dblsptrmap.find(STDDEV) != dblsptrmap.end()) return vector<double>(*dblsptrmap.at(STDDEV));
    return vector<double>();
}
vector<double>  IData::getTriggerIntv(){
    if (dblsptrmap.find(TRIGGERINTV) != dblsptrmap.end()) return vector<double>(*dblsptrmap.at(TRIGGERINTV));
    return vector<double>();
}

} // namespace end
