#include <string.h>
#include <stdlib.h>
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
        lastint = INT_MIN;                                // darf nur bei doLastFill verändert werden
        lastdbl = NAN;
        laststring = "NaN";
        doLastfill = false;
        unsigned int snapidx=0, srcidx=0;
        int tgtpos=0, snappos=0, srcpos= 0;
        int lastLoaded = 0;
        bool snapAtEnd = true;
        bool srcAtEnd = true;
        vector<int>& src = data.posCounts;
        vector<int> snap;

        // init
        if (((fillType == LastFill) || (fillType == LastNANFill)) && (data.getDeviceType() == Axis)) doLastfill = true;
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

        if ((snapdata != NULL) && (!snapdata->isArrayData())) snap = (*snapdata).posCounts;
        if (src.size() > 0) srcAtEnd = false;
        if (snap.size() > 0) snapAtEnd = false;

        for (unsigned int tgtidx=0; tgtidx < posrefs.size(); ++tgtidx){
            tgtpos = posrefs[tgtidx];
            if (!srcAtEnd) srcpos = src[srcidx];
            if (!snapAtEnd) snappos = snap[snapidx];

            // check if snapshot positions should be loaded as Lastfill value
            while ((tgtpos > snappos) && !snapAtEnd && doLastfill) {
                //  load the snapshot positions to fill lastPosition
                if (snappos > lastLoaded) {
                    loadLast(snapidx, *snapdata);
                    lastLoaded = snappos;
                }
                ++ snapidx;
                if (snapidx >= snap.size()) {
                    --snapidx;
                    snapAtEnd = true;
                }
                snappos = snap[snapidx];
            }

            // check if previous positions should be loaded as Lastfill value
            while ((tgtpos > srcpos) && !srcAtEnd) {
                    if (doLastfill && (srcpos >= lastLoaded)) {
                        loadLast(srcidx, data);
                        lastLoaded = srcpos;
                    }
                    ++ srcidx;
                    if (srcidx >= src.size()) {
                        --srcidx;
                        srcAtEnd = true;
                    }
                    srcpos = src[srcidx];
            }

            if (tgtpos < srcpos) {
                setNanOrLast(tgtidx);
            }
            else if (tgtpos == srcpos){
                unsigned int usedindex = srcidx;
                for (int j : intarrs){
                    intsptrmap.at(j)->at(tgtidx) = data.intsptrmap.at(j)->at(usedindex);
                    if (doLastfill && (j == 0)) lastint = data.intsptrmap.at(j)->at(usedindex);
                }
                for (int j : dblarrs){
                    dblsptrmap.at(j)->at(tgtidx) = data.dblsptrmap.at(j)->at(usedindex);
                    if (doLastfill && (j == 0)) lastdbl = data.dblsptrmap.at(j)->at(usedindex);
                }
                for (int j : strarrs){
                    strsptrmap.at(j)->at(tgtidx) = data.strsptrmap.at(j)->at(usedindex);
                    if (doLastfill && (j == 0)) laststring = data.strsptrmap.at(j)->at(usedindex);
                }
                ++ srcidx;
                if (srcidx >= src.size()) {
                    --srcidx;
                    srcAtEnd = true;
                }
            }
            else if (tgtpos > srcpos) {
                setNanOrLast(tgtidx);
            }
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

void IData::setNanOrLast(int index)
{
    for(auto const &vpair : intsptrmap){
        int j = vpair.first;
        if(doLastfill && (j == 0))
            intsptrmap.at(j)->at(index) = lastint;
        else
            intsptrmap.at(j)->at(index) = INT_MIN;
    }
    for (auto const &vpair : dblsptrmap){
        int j = vpair.first;
        if(doLastfill && (j == 0))
            dblsptrmap.at(j)->at(index) = lastdbl;
        else
            dblsptrmap.at(j)->at(index) = NAN;
    }
    for (auto const &vpair : strsptrmap){
        int j = vpair.first;
        if(doLastfill && (j == 0))
            strsptrmap.at(j)->at(index) = laststring;
        else
            strsptrmap.at(j)->at(index) = "NaN";
    }
}

void IData::loadLast(unsigned int index, IData& data)
{
    if (intsptrmap.find(0) != intsptrmap.end()) lastint = data.intsptrmap.at(0)->at(index);
    if (dblsptrmap.find(0) != dblsptrmap.end()) lastdbl = data.dblsptrmap.at(0)->at(index);
    if (strsptrmap.find(0) != strsptrmap.end()) laststring = data.strsptrmap.at(0)->at(index);
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
