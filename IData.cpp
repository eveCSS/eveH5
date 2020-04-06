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
        lastint = INT_MIN;
        lastdbl = NAN;
        laststring = "NaN";
        doLastfill = false;
        unsigned int snapidx=0, srcidx=0;
        int tgtpos=0, snappos=0, srcpos= 0;
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

        set<unsigned int> dPosCounts(data.posCounts.begin(), data.posCounts.end());
        // find double poscount values to apply doublePosCount workaround
        if ((data.getDeviceType() == Axis) && (dPosCounts.size() != data.posCounts.size())){
           cout << "Warning: posrefs for axis " << data.getId() << " are not unique, applying doublePosRef workaround" << endl;
           set<unsigned int> dPC = dPosCounts;
           for (int newpc : data.posCounts) {
               if (dPC.erase(newpc) != 1) doublePosCounts.insert(newpc);
            }
        }

        for (unsigned int tgtidx=0; tgtidx < posrefs.size(); ++tgtidx){
            tgtpos = posrefs[tgtidx];
            if (!srcAtEnd) srcpos = src[srcidx];
            if (srcpos < 0) srcpos = 0;
            if (!snapAtEnd) snappos = snap[snapidx];
            if (snappos < 0) snappos = 0;

            if (tgtpos < srcpos) {
                if (doLastfill) {
                    while ((tgtpos > snappos) && !snapAtEnd) {
                        loadLast(snapidx, *snapdata);
                        ++snapidx;
                        if (snapidx >= snap.size()) {
                            --snapidx;
                            snapAtEnd = true;
                            break;
                        }
                        snappos = snap[snapidx];
                    }
                }
                setNanOrLast(tgtidx);
            }
            else if (tgtpos == srcpos){
                unsigned int usedindex = srcidx;
                // apply workaround for doublePosCount
                // take the last value for axes
                if ((data.getDeviceType() == Axis) &&
                                (doublePosCounts.find(data.posCounts[srcidx]) != doublePosCounts.end())) {
                    for (unsigned int dindex = data.posCounts.size()-1; dindex > srcidx; --dindex){
                        if (data.posCounts[dindex] == data.posCounts[srcidx]) {
                            usedindex = dindex;
                            break;
                        }
                    }
                }
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
                // skip non-monotone values
                 while (src[srcidx] < srcpos){
                     if (doublePosCounts.find(src[srcidx]) == doublePosCounts.end())
                         cout << "Error: posrefs for axis " << data.getId() << " are not monotone, skip value" << endl;
                     if (srcAtEnd) {
                         --srcidx;
                         break;
                     }
                     ++ srcidx;
                     if (srcidx >= src.size()) {
                         --srcidx;
                         srcAtEnd = true;
                     }
                 }
            }
            else if (tgtpos > srcpos) {
                if (doLastfill) {
                    while (((tgtpos > snappos) && !snapAtEnd) || ((tgtpos > srcpos) && !srcAtEnd))  {
                        if ((snappos < srcpos) && !snapAtEnd){
                            loadLast(snapidx, *snapdata);
                            ++ snapidx;
                            if (snapidx >= snap.size()) {
                                --snapidx;
                                snapAtEnd = true;
                            }
                            snappos = snap[snapidx];
                        }
                        else if (!srcAtEnd) {
                            loadLast(srcidx, data); // setLast macht workaround for doublePosCount
                            ++ srcidx;
                            if (srcidx >= src.size()) {
                                --srcidx;
                                srcAtEnd = true;
                            }
                           // skip non-monotone values
                            while (src[srcidx] < srcpos){
                                if (doublePosCounts.find(src[srcidx]) == doublePosCounts.end())
                                    cout << "Error: posrefs for axis " << data.getId() << " are not monotone, skip value" << endl;
                                if (srcAtEnd) {
                                    --srcidx;
                                    break;
                                }
                                ++ srcidx;
                                if (srcidx >= src.size()) {
                                    --srcidx;
                                    srcAtEnd = true;
                                }
                            }
                            srcpos = src[srcidx];
                        }
                        // ((tgtpos > snappos) && !snapAtEnd) => true
                        else {
                            loadLast(snapidx, *snapdata);
                            ++ snapidx;
                            if (snapidx >= snap.size()) {
                                --snapidx;
                                snapAtEnd = true;
                            }
                            snappos = snap[snapidx];
                        }
                    }
                }
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
    unsigned int usedindex = index;
    // apply workaround for doublePosCount
    // take the last value for axes
    if (doublePosCounts.find(data.posCounts[index]) != doublePosCounts.end()) {
        for (unsigned int dindex = data.posCounts.size()-1; dindex > index; --dindex){
            if (data.posCounts[dindex] == data.posCounts[index]) {
                usedindex = dindex;
                break;
            }
        }
    }
    if (intsptrmap.find(0) != intsptrmap.end()) lastint = data.intsptrmap.at(0)->at(usedindex);
    if (dblsptrmap.find(0) != dblsptrmap.end()) lastdbl = data.dblsptrmap.at(0)->at(usedindex);
    if (strsptrmap.find(0) != strsptrmap.end()) laststring = data.strsptrmap.at(0)->at(usedindex);
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
