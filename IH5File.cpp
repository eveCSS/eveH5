#include <string.h>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <algorithm>    // std::sort
#include <set>
#include "IH5File.h"
#include <H5Exception.h>

using namespace std;

#define STHROW(msg) { \
     ostringstream err;\
     err<<msg; \
     throw runtime_error(err.str()); }

struct intstruct {
    int posCount;
    int data;
};
struct dblstruct {
    int posCount;
    double data;
};
struct twodblstruct {
    int posCount;
    double data1;
    double data2;
};
struct strstruct {
    int posCount;
    char data[41];
};
struct enumstruct {
    int posCount;
    char data[17];
};
#define STR_STRUCT_SIZE 45
#define ENUM_STRUCT_SIZE 21

namespace eve {

// expects an already opened h5 Filehandle
IH5File::IH5File(H5::H5File oh5file, string file, float version)
{
    selectedChain = 0;
    filename = file;
    isOpen = true;
    chainList.clear();
    h5file = oh5file;
    h5version = version;
    sections = {"", "meta"};
    calculations = {""};
    normalizations = {"normalized"};
    chainTSfullname = "meta/PosCountTimer";
    timestampMeta = NULL;
}

void IH5File::init()
{
    Group root;

    openGroup(root, "/");
    rootAttributes = getH5Attributes(root);
    chainList = getNumberGroups(root);

    setChain(1);
    if (haveGroupWithName(root, "device")){
        Group devices;
        openGroup(devices, "/device");
        try {
            parseDatasets(devices, "/device", monitormeta, "", Monitor);
        }
        catch (Exception error){
            STHROW("Error parsing file " << filename << "; H5 Error: " << error.getDetailMsg() );
        }
        closeGroup(devices);
    }
    closeGroup(root);
}

IH5File::~IH5File()
{
//    close();
    if (isOpen) h5file.close();
    isOpen = false;
    chainList.clear();
    rootAttributes.clear();
    chainAttributes.clear();
    for (IMetaData* mdata: chainmeta) delete mdata;
    chainmeta.clear();
    for (IMetaData* mdata: extensionmeta) delete mdata;
    extensionmeta.clear();
    for (IMetaData* mdata: monitormeta) delete mdata;
    monitormeta.clear();
    if (timestampMeta != NULL) delete timestampMeta;
/*  do not throw exceptions in destructor
    try {
        h5file.close();
    }
    catch (Exception error){
        STHROW("Error closing file; H5 Error: " << error.getDetailMsg() );
    }
*/
}

bool IH5File::isChainSection(string name){
    if (sections.find(name) != sections.end())
        return true;
    return false;
}

bool IH5File::isCalc(string name){
    if (calculations.find(name) != calculations.end())
        return true;
    return false;
}

bool IH5File::isNormalization(string name){
    if (normalizations.find(name) != normalizations.end())
        return true;
    return false;
}

//void IH5File::close()
//{
//    isOpen = false;
//    chainList.clear();
//    rootAttributes.clear();
//    chainAttributes.clear();
//    try {
//        h5file.close();
//    }
//    catch (Exception error){
//        STHROW("Error closing file; H5 Error: " << error.getDetailMsg() );
//    }
//}

string IH5File::getSectionString(Section sect){
    string chainname = "/c" + to_string(selectedChain);
    switch (sect) {
    case Standard:
        return chainname +"";
        break;
    case Snapshot:
        return "";
        break;
    case Monitor:
        return "/device";
        break;
    case Timestamp:
        return chainname + "/meta";
        break;
    default:
        break;
    }
    return "unknown";
}

void IH5File::setChain(int chain){

    for (vector<int>::iterator cit=chainList.begin(); cit != chainList.end(); ++cit){
        if (*cit == chain){
            if (selectedChain != chain){
                selectedChain = chain;
                chainInventory();
            }
            return;
        }
    }
}

void IH5File::chainInventory(){

    bool doneLog = false;
    Group chain;
    string path = "/c"+to_string(selectedChain);
    openGroup(chain, path);
    chainAttributes = getH5Attributes(chain);

    for (IMetaData* mdata : chainmeta) delete mdata;
    chainmeta.clear();
    for (IMetaData* mdata : extensionmeta) delete mdata;
    extensionmeta.clear();

    chainTSfullname = path + "/" + chainTSfullname;
    if (timestampMeta != NULL) delete timestampMeta;
    timestampMeta = NULL;

    vector<string> secgroups = getGroups(chain);
    // we may have no sections at all
    secgroups.push_back("");

    for (vector<string>::iterator sit=secgroups.begin(); sit != secgroups.end(); ++sit){
//        cout << "chainInventory test section: " << *sit << endl;
        if (!isChainSection(*sit)) continue;
        Section current_section = Standard;

        string secpath = path;
        if (sit->size() != 0) secpath += "/" + *sit;

        Group secgrp;
        openGroup(secgrp, secpath);
//        cout << "chainInventory section: " << secpath << endl;
        if (getSectionString(Snapshot) == secpath) current_section = Snapshot;
        parseDatasets(secgrp, secpath, chainmeta, "", current_section);
        parseGroupDatasets(secgrp, secpath, chainmeta, "", current_section);
        vector<string> dsgroups = getGroups(secgrp);
        for (vector<string>::iterator it=dsgroups.begin(); it != dsgroups.end(); ++it){
            string grpath = *it;
            // cout << "chainInventory group: " << grpath << endl;
            string calcpath = secpath + "/" + grpath;
            Group calcgr;

            if (isNormalization(grpath)){
//                cout << "chainInventory normalized group: " << calcpath << " / " << grpath << endl;
                openGroup(calcgr, calcpath);
                parseDatasets(calcgr, secpath, chainmeta, grpath, current_section);
                parseGroupDatasets(calcgr, calcpath, chainmeta, grpath, current_section);
                closeGroup(calcgr);
            }
            else if (isCalc(grpath)){
//                cout << "chainInventory calc group: " << calcpath << endl;
                openGroup(calcgr, calcpath);
                parseDatasets(calcgr, secpath, extensionmeta, grpath, current_section);
                parseGroupDatasets(calcgr, calcpath, extensionmeta, grpath, current_section);
                closeGroup(calcgr);
            }
            else {
                // ignore meta for EVEH5 version 1.0 with empty sections
                if ((h5version == 1.0) && !((sit->size() == 0) && (grpath == "meta"))){
                    if (!doneLog) cout << "Caution! ignoring non-raw data" << endl;
                    doneLog = true;
                }
            }
        }
        closeGroup(secgrp);
    }
    closeGroup(chain);
}

vector<MetaData *> IH5File::getMetaData(Section section, string id, string name){
    vector<MetaData *> result;
    string path = getSectionString(section);
    if (path.size() == 0) {
        return result;
    }

    if (section == Timestamp){
        if (timestampMeta != NULL) result.push_back(new IMetaData(*timestampMeta));
    }
    else if (section == Monitor)
        result = getMetaData(&monitormeta, path, id, name);
    else
        result = getMetaData(&chainmeta, path, id, name);

    return result;
}

string IH5File::getNameById(Section section, string id){

    string path = getSectionString(section);
    if (path.empty() || id.empty())
        return string();

    if (section == Timestamp){
        if (timestampMeta != NULL) return timestampMeta->getName();
    }
    else if (section == Monitor)
        return getNameById(&monitormeta, path, id);
    else
        return getNameById(&chainmeta, path, id);

    return string();
}

string IH5File::getNameById(vector<IMetaData *> *devlist, string path, string id){

    for (vector<IMetaData *>::iterator it=devlist->begin(); it != devlist->end(); ++it){
        IMetaData* mdat = *it;
        if (mdat->getPath().find(path)==0){
            if ((mdat->getId() == id) && (!mdat->getName().empty()))
                return mdat->getName();
        }
    }
    return string();
}

vector<MetaData *> IH5File::getMetaData(vector<IMetaData *> *devlist, string path, string id, string name){
    vector<MetaData *> result;
    if (path.size() == 0) {
        return result;
    }

    for (vector<IMetaData *>::iterator it=devlist->begin(); it != devlist->end(); ++it){
        IMetaData* mdat = *it;
        if (mdat->getPath().find(path)==0){
            if (id.length() > 0){
                if (mdat->getId() == id) result.push_back(new IMetaData(*mdat));
            }
            else if (name.length() > 0){
                if (mdat->getName() == name) result.push_back(new IMetaData(*mdat));
            }
            else
                result.push_back(new IMetaData(*mdat));
        }
    }
    return result;
}

MetaData* IH5File::findMetaData(vector<IMetaData*>& mdlist, string fullh5name){
    for (vector<IMetaData *>::iterator it=mdlist.begin(); it != mdlist.end(); ++it){
        IMetaData* mdat = *it;
        string name = mdat->getFQH5Name();
        // cout << "compare: " << name << " with " << fullh5name << endl;
        if (name.compare(fullh5name) == 0){
            return mdat;
        }
    }
    return NULL;
}

void IH5File::openGroup(Group& h5group, string path){
    try {
        h5group = h5file.openGroup(path);
    }
    catch (Exception error){
        STHROW("Error opening group " << path << "; H5 Error: " << error.getDetailMsg() );
    }
}

void IH5File::closeGroup(Group& h5group){
    try {
        h5group.close();
    }
    catch (Exception error){
        STHROW("Error closing group; H5 Error: " << error.getDetailMsg() );
    }
}

// return the names of all groups in group
vector<string> IH5File::getGroups(Group& group){
    vector<string> groupList;
    for (hsize_t index = 0; index < group.getNumObjs(); ++index){
        string objname = group.getObjnameByIdx(index);
        if (group.getObjTypeByIdx(index) == H5G_GROUP){
            groupList.push_back(objname);
        }
    }
    return groupList;
}

// collect all groups with array data i.e. with attribute "XML-ID"
void IH5File::parseGroupDatasets(Group& group, string prefix, vector<IMetaData*>& imeta, string calctype, Section section)
{
    
    vector<string> dsgroups = getGroups(group);
    for (vector<string>::iterator it=dsgroups.begin(); it != dsgroups.end(); ++it){
        if (isCalc(*it) || isNormalization(*it)) continue;
        Group dsgroup;
        string fullname = prefix + "/" + *it;
        if (calctype.length() > 0) fullname = prefix + "/" + calctype + "/" + *it;
        openGroup(dsgroup, fullname);

        // ignore all groups without an attribute "XML-ID" (could be unsupported calc groups)
        map<string, string> attribs = getH5Attributes(dsgroup);
        if (attribs.count("XML-ID") == 0) continue;

        IMetaData* dinfo = new IMetaData(prefix + "/", calctype, *it, section, attribs);
        for (hsize_t index = 0; index < dsgroup.getNumObjs(); ++index){
            if (dsgroup.getObjTypeByIdx(index) != H5G_DATASET) continue;
//            string subname = dsgroup.getObjnameByIdx(index);
//            int posCnt = strtol(subname.c_str(),NULL,10);
//            dinfo->posCounts.push_back(posCnt);
            // open the first dataset to retrieve the datatype
            if (index  == 0) {
                string subname = dsgroup.getObjnameByIdx(index);
                DataSet ds = dsgroup.openDataSet(subname);
                dinfo->setDataType(ds);
                ds.close();
            }
            ++dinfo->dim0;
        }
//        sort(begin(dinfo->posCounts), end(dinfo->posCounts));
        dinfo->dstype = EVEDSTArray;
        imeta.push_back(dinfo);
        closeGroup(dsgroup);
    }
}



void IH5File::parseDatasets(Group& group, string prefix, vector<IMetaData*>& imeta, string calctype, Section section){

    for (hsize_t index = 0; index < group.getNumObjs(); ++index){
        string objname = group.getObjnameByIdx(index);
        if (group.getObjTypeByIdx(index) == H5G_DATASET){
            DataSet ds = group.openDataSet(objname);
            Section useSection=section;

            if (prefix+"/"+objname == chainTSfullname) useSection=Timestamp;

            IMetaData* dinfo = new IMetaData(prefix + "/", calctype, objname, useSection, getH5Attributes(ds));
            dinfo->setDataType(ds);
            if (useSection==Timestamp){
                if (timestampMeta != NULL) delete timestampMeta;
                timestampMeta = dinfo;
            }
            else
                imeta.push_back(dinfo);
            ds.close();

            // cout << "parseDatasets DS: " << dinfo->getId() << " full path: " << dinfo->getFQH5Name() << endl;

        }
    }
}

// return the names of all groups in group, where groupnames have a leading "c" and can be converted to numbers
vector<int> IH5File::getNumberGroups(Group& group){
    vector<int> groupList;
    for (hsize_t index = 0; index < group.getNumObjs(); ++index){
        string objname = group.getObjnameByIdx(index);
        if ((group.getObjTypeByIdx(index) == H5G_GROUP) && (objname.size() > 1) && (objname[0] == 'c')){
            try {

                groupList.push_back(stoi(objname.substr(1), nullptr, 10));
            }
            catch (Exception error){

            }
        }
    }
    return groupList;
}

bool IH5File::haveGroupWithName(Group& group, string name){
    vector<string> grouplist = getGroups(group);
    for (vector<string>::iterator it=grouplist.begin(); it != grouplist.end(); ++it){
        if ((*it).compare(name) == 0) return true;
    }
    return false;
}

map<string, string> IH5File::getH5Attributes(H5Object& object){

    map<string, string> attribMap;
    unsigned int numAttrib = object.getNumAttrs();
    pair<map<string,string>::iterator,bool> ret;

    for (unsigned int index = 0; index < numAttrib; ++index){
        Attribute attrib = object.openAttribute(index);
        string name = attrib.getName();
        StrType stread = StrType(attrib.getStrType());
        int length = stread.getSize();
        char *strg_C = new char[(size_t)length+1];
        attrib.read(stread, strg_C);
        strg_C[length] = 0;
        ret=attribMap.insert ( pair<string, string>(name, string(strg_C)) );
        delete[] strg_C;
        attrib.close();
    }
    return attribMap;
}

Data *IH5File::getData(MetaData *dInfo){

    // ... siehe unten: getData(IMetaData* dInfo)
    IData* data = new IData((IMetaData&)*dInfo);
    if (((IMetaData*)dInfo)->dstype == EVEDSTArray){
        readDataArray(data);
    }
    else if (((IMetaData*)dInfo)->dstype == EVEDSTPCOneColumn){
        readDataPCOneCol(data);
    }
    else if (((IMetaData*)dInfo)->dstype == EVEDSTPCTwoColumn){
        readDataPCTwoCol(data);
    }
    else {
        STHROW("Unable to read data: unknown DataSet type");
    }

    // check if we have averagedata
    addExtensionData(data);

    return data;
}

void IH5File::readDataArray(IData* data){

    hsize_t dims_out[2];
    size_t element_size;

    Group dsgroup;
    openGroup(dsgroup, data->getFQH5Name());

    for (hsize_t index = 0; index < dsgroup.getNumObjs(); ++index){
        if (dsgroup.getObjTypeByIdx(index) != H5G_DATASET) continue;
        string subname = dsgroup.getObjnameByIdx(index);
        string objname = data->getFQH5Name() + "/" + subname;
        int posCnt = strtol(subname.c_str(),NULL,10);
        data->posCounts.push_back(posCnt);
        DataSet h5dset;
        H5::DataType h5dtype;
        try {
            h5dset = dsgroup.openDataSet(subname);
            h5dset.getSpace().getSimpleExtentDims( dims_out, NULL);
            h5dtype = h5dset.getDataType();
            element_size = h5dtype.getSize();
        }
        catch (DataSetIException error){
            STHROW("H5 DataSet error while reading Dataset " << objname << " H5 Error: " << error.getDetailMsg() );
        }
        catch (DataTypeIException error){
            STHROW("H5 Datatype error while reading Dataset " << objname << " H5 Error: " << error.getDetailMsg() );
        }
        catch (DataSpaceIException error){
            STHROW("Error reading H5 Dataspace in Dataset " << objname << " H5 Error: " << error.getDetailMsg() );
        }
        catch (FileIException error){
            STHROW("Unable to open H5 DataSet: " << objname << " H5 Error: " << error.getDetailMsg() );
        }
        catch (...){
            STHROW("Unknown H5 Error in Dataset " << objname);
        }
        if ((dims_out[1] > 1) || (data->h5dimensions[0] != dims_out[0]) || (data->h5dimensions[1] != dims_out[1])){
            STHROW("Error: Unexpected dimension in Dataset " << objname);
        }

        shared_ptr<char> memBuffer(new char[element_size * dims_out[0]], default_delete<char[]>() );
        // std::shared_ptr<char> memBuffer(new char[element_size * dims_out[0]], array_deleter<char>() );

        if (memBuffer.get() == NULL){
            STHROW("Unable to allocate memory when reading Dataset " << objname);
        }
        try {
            h5dset.read(memBuffer.get(), h5dtype);
        }
        catch (DataSetIException error){
            STHROW("Error reading Dataset " << objname << " H5 Error: " << error.getDetailMsg() );
        }
        catch (...){
            STHROW("Unknown error while reading DataSet: " << objname);
        }
        data->posPtrHash.insert(pair<int, shared_ptr<char>>(posCnt, memBuffer));
    }
    closeGroup(dsgroup);
    sort(begin(data->posCounts), end(data->posCounts));


}

bool IH5File::insertData(IData* data, int idx, char* memptr, int element_size, bool insert){

    if (insert){
        if (data->datatype == DTint32) {
            data->intsptrmap.at(INTVECT1)->insert(next(data->intsptrmap.at(INTVECT1)->begin(),idx),  *((int*)(memptr + 4)));
        }
        else if (data->datatype == DTuint32) {
            data->intsptrmap.at(INTVECT1)->insert(next(data->intsptrmap.at(INTVECT1)->begin(),idx),  (int)*((unsigned int*)(memptr + 4)));
        }
        else if (data->datatype == DTint8) {
            data->intsptrmap.at(INTVECT1)->insert(next(data->intsptrmap.at(INTVECT1)->begin(),idx),  (int)*(memptr + 4));
        }
        else if (data->datatype == DTuint8) {
            data->intsptrmap.at(INTVECT1)->insert(next(data->intsptrmap.at(INTVECT1)->begin(),idx),  (int)*((unsigned char*)(memptr + 4)));
        }
        else if (data->datatype == DTint16) {
            data->intsptrmap.at(INTVECT1)->insert(next(data->intsptrmap.at(INTVECT1)->begin(),idx),  (int)*((short*)(memptr + 4)));
        }
        else if (data->datatype == DTuint16) {
            data->intsptrmap.at(INTVECT1)->insert(next(data->intsptrmap.at(INTVECT1)->begin(),idx),  (int)*((unsigned short*)(memptr + 4)));
        }
        else if (data->datatype == DTfloat32) {
            data->dblsptrmap.at(DBLVECT1)->insert(next(data->dblsptrmap.at(DBLVECT1)->begin(),idx),  ((double)*((float*)(memptr + 4))));
        }
        else if (data->datatype == DTfloat64) {
            data->dblsptrmap.at(DBLVECT1)->insert(next(data->dblsptrmap.at(DBLVECT1)->begin(),idx),  *((double*)(memptr + 4)));
        }
        else if (data->datatype == DTstring) {
            string tmpstring(memptr + 4, element_size-4);
            data->strsptrmap.at(STRVECT1)->insert(next(data->strsptrmap.at(STRVECT1)->begin(),idx),  string(tmpstring.c_str()));
        }
        else
            return  true;
    }
    else {
        if (data->datatype == DTint32) {
            data->intsptrmap.at(INTVECT1)->at(idx) = *((int*)(memptr + 4));
        }
        else if (data->datatype == DTuint32) {
            data->intsptrmap.at(INTVECT1)->at(idx) = (int)*((unsigned int*)(memptr + 4));
        }
        else if (data->datatype == DTint8) {
            data->intsptrmap.at(INTVECT1)->at(idx) = (int)*(memptr + 4);
        }
        else if (data->datatype == DTuint8) {
            data->intsptrmap.at(INTVECT1)->at(idx) = (int)*((unsigned char*)(memptr + 4));
        }
        else if (data->datatype == DTint16) {
            data->intsptrmap.at(INTVECT1)->at(idx) = (int)*((short*)(memptr + 4));
        }
        else if (data->datatype == DTuint16) {
            data->intsptrmap.at(INTVECT1)->at(idx) = (int)*((unsigned short*)(memptr + 4));
        }
        else if (data->datatype == DTfloat32) {
            data->dblsptrmap.at(DBLVECT1)->at(idx) = ((double)*((float*)(memptr + 4)));
        }
        else if (data->datatype == DTfloat64) {
            data->dblsptrmap.at(DBLVECT1)->at(idx) = *((double*)(memptr + 4));
        }
        else if (data->datatype == DTstring) {
            string tmpstring(memptr + 4, element_size-4);
            data->strsptrmap.at(STRVECT1)->at(idx) = string(tmpstring.c_str());
        }
        else
            return true;
    }
    return false;
}

void IH5File::readDataPCOneCol(IData* data){

    if (data->datatype == DTunknown) STHROW("unsupported datatype");

    hsize_t dims_out[2];
    size_t element_size;
    DataSet h5dset;
    H5::DataType h5dtype;
    string objname = data->getFQH5Name();

    try {
        h5dset = h5file.openDataSet(objname);
        h5dset.getSpace().getSimpleExtentDims( dims_out, NULL);
        h5dtype = h5dset.getDataType();
        element_size = h5dtype.getSize();
    }
    catch (DataSetIException error){
        STHROW("H5 DataSet error while reading Dataset " << objname << " H5 Error: " << error.getDetailMsg() );
    }
    catch (DataTypeIException error){
        STHROW("H5 Datatype error while reading Dataset " << objname << " H5 Error: " << error.getDetailMsg() );
    }
    catch (DataSpaceIException error){
        STHROW("Error reading H5 Dataspace in Dataset " << objname << " H5 Error: " << error.getDetailMsg() );
    }
    catch (FileIException error){
        STHROW("Unable to open H5 DataSet: " << objname << " H5 Error: " << error.getDetailMsg() );
    }
    catch (...){
        STHROW("Unknown H5 Error in Dataset " << objname);
    }
    if (data->h5dimensions[0] != dims_out[0])
        STHROW("Unsupported dimension error in Dataset " << objname);

    switch (data->datatype) {
    case DTint8:
    case DTuint8:
        if (element_size != 5)  STHROW("8 bit size error in Dataset " << objname);
        break;
    case DTuint16:
    case DTint16:
        if (element_size != 6)  STHROW("16 bit size error in Dataset " << objname);
        break;
    case DTint32:
    case DTuint32:
    case DTfloat32:
        if (element_size != 8)  STHROW("32bit size error in Dataset " << objname);
        break;
    case DTfloat64:
        if (element_size != 12)  STHROW("double size error in Dataset " << objname);
        break;
    case DTstring:
        if (element_size < 5)  STHROW("string size error in Dataset " << objname);
        break;
    default:
        break;
    }

    void* memBuffer = malloc(element_size * dims_out[0]);
    if (memBuffer == NULL)
        STHROW("Unable to allocate memory when reading Dataset " << objname);
    try {
        h5dset.read(memBuffer, h5dtype);
    }
    catch (DataSetIException error){
        STHROW("Error reading Dataset " << objname << " H5 Error: " << error.getDetailMsg() );
    }
    catch (...){
        STHROW("Unknown error while reading DataSet: " << objname);
    }

    if (data->datatype == DTstring)
        data->strsptrmap.insert(pair<int, shared_ptr<vector<string>>>(STRVECT1, make_shared<vector<string>>(dims_out[0])));
    else if ((data->datatype == DTfloat64) || (data->datatype == DTfloat32))
        data->dblsptrmap.insert(pair<int, shared_ptr<vector<double>>>(DBLVECT1, make_shared<vector<double>>(dims_out[0])));
    else
        data->intsptrmap.insert(pair<int, shared_ptr<vector<int>>>(INTVECT1, make_shared<vector<int>>(dims_out[0])));

    bool doSort = true;                                   // sort all values with posrefs
    if (data->getSection() == Monitor) doSort = false;    // don't sort monitor data
    bool typeerror = false;
    char *memptr;
    int highestPosCount = -1;
    bool negativeError = false;
    bool doublePCError = false;
    char *base_memptr = (char*)memBuffer;
    int newdim = 0;
    for (unsigned int i = 0; i < dims_out[0]; ++i){
        memptr = base_memptr + (element_size * i);
        int posCount = *((int*)memptr);

        if (doSort && (posCount < 0)) {
            negativeError = true;
            continue;
        }

        if (doSort && (posCount <= highestPosCount)) {
            // find correct position for sorted posCounts
            int idx = 0;
            while (data->posCounts.at(idx) < posCount) {++idx;}

            if (data->posCounts.at(idx) == posCount){
                doublePCError = true;
                if (data->getDeviceType() == Axis){
                    // replace at idx
                    typeerror = insertData(data, idx, memptr, element_size, false);
                }
            }
            else {
                // insert at idx
                data->posCounts.insert(next(data->posCounts.begin(),idx), posCount);
                typeerror = insertData(data,idx, memptr, element_size, true);
                ++newdim;
            }
        }
        else {
            // replace at newdim
            // TODO
            data->posCounts.push_back(posCount);
            typeerror = insertData(data,newdim, memptr, element_size, false);
            ++newdim;
            highestPosCount = posCount;
        }
    }
    if (negativeError)
        cout << "Warning: Skipped values with negative PosCounter" << endl;
    if (doublePCError)
        cout << "Warning: posrefs for axis " << data->getId() << " are not unique, applied doublePosRef workaround" << endl;
    if (typeerror)
        STHROW("Unable to read data: unknown datatype" );

    if (data->datatype == DTstring)
        data->strsptrmap.at(STRVECT1)->resize(newdim);
    else if ((data->datatype == DTfloat64) || (data->datatype == DTfloat32))
        data->dblsptrmap.at(DBLVECT1)->resize(newdim);
    else
        data->intsptrmap.at(INTVECT1)->resize(newdim);
    free(memBuffer);

}

void IH5File::readDataPCTwoCol(IData* data){

    if ((data->datatype != DTint32) && (data->datatype != DTfloat64))
        STHROW("unsupported datatype, currently only int32 and float64 is supported for two column arrays");

    hsize_t dims_out[2];
    size_t element_size;
    DataSet h5dset;
    H5::DataType h5dtype;
    string objname = data->getFQH5Name();

    try {
        h5dset = h5file.openDataSet(objname);
        h5dset.getSpace().getSimpleExtentDims( dims_out, NULL);
        h5dtype = h5dset.getDataType();
        element_size = h5dtype.getSize();
    }
    catch (DataSetIException error){
        STHROW("H5 DataSet error while reading Dataset " << objname << " H5 Error: " << error.getDetailMsg() );
    }
    catch (DataTypeIException error){
        STHROW("H5 Datatype error while reading Dataset " << objname << " H5 Error: " << error.getDetailMsg() );
    }
    catch (DataSpaceIException error){
        STHROW("Error reading H5 Dataspace in Dataset " << objname << " H5 Error: " << error.getDetailMsg() );
    }
    catch (FileIException error){
        STHROW("Unable to open H5 DataSet: " << objname << " H5 Error: " << error.getDetailMsg() );
    }
    catch (...){
        STHROW("Unknown H5 Error in Dataset " << objname);
    }
    if (data->h5dimensions[0] != dims_out[0])
        STHROW("Unexpected dimension error in Dataset " << objname);
    if ((element_size != 20) && (element_size != 12))
        STHROW("Unexpected string size error in Dataset " << objname);
    void* memBuffer = malloc(element_size * dims_out[0]);
    if (memBuffer == NULL)
        STHROW("Unable to allocate memory when reading Dataset " << objname);
    try {
        h5dset.read(memBuffer, h5dtype);
    }
    catch (DataSetIException error){
        STHROW("Error reading Dataset " << objname << " H5 Error: " << error.getDetailMsg() );
    }
    catch (...){
        STHROW("Unknown error while reading DataSet: " << objname);
    }

    if (data->datatype == DTint32){
        data->intsptrmap.insert(pair<int, shared_ptr<vector<int>>>(INTVECT1, make_shared<vector<int>>(dims_out[0])));
        data->intsptrmap.insert(pair<int, shared_ptr<vector<int>>>(INTVECT2, make_shared<vector<int>>(dims_out[0])));
    }
    else {
        data->dblsptrmap.insert(pair<int, shared_ptr<vector<double>>>(DBLVECT1, make_shared<vector<double>>(dims_out[0])));
        data->dblsptrmap.insert(pair<int, shared_ptr<vector<double>>>(DBLVECT2, make_shared<vector<double>>(dims_out[0])));
    }

    int highestPosCount = -1;
    bool negativeError = false;
    bool doublePCError = false;
    char *memptr;
    char *base_memptr = (char*)memBuffer;
    int newdim = 0;
    for (unsigned int i = 0; i < dims_out[0]; ++i){
        memptr = base_memptr + (element_size * i);
        int posCount = *((int*)memptr);

        if (posCount < 0) {
            negativeError = true;
            continue;
        }

        if (posCount <= highestPosCount) {
            // find correct position for sorted posCounts
            int idx = 0;
            while (data->posCounts.at(idx) < posCount) {++idx;}

            if (data->posCounts.at(idx) == posCount){
                doublePCError = true;
            }
            else {
                // insert at idx
                data->posCounts.insert(next(data->posCounts.begin(),idx), posCount);
                if (data->datatype == DTint32){
                    int target;
                    memcpy((void*)&target, memptr + 4, 4);
                    data->intsptrmap.at(INTVECT1)->insert(next(data->intsptrmap.at(INTVECT1)->begin(),idx),  target);
                    memcpy((void*)&target, memptr + 8, 4);
                    data->intsptrmap.at(INTVECT2)->insert(next(data->intsptrmap.at(INTVECT2)->begin(),idx), target);
                }
                else {
                    double target;
                    memcpy((void*)&target, memptr + 4, 8);
                    data->dblsptrmap.at(DBLVECT1)->insert(next(data->dblsptrmap.at(DBLVECT1)->begin(),idx),  target);
                    memcpy((void*)&target, memptr + 12, 8);
                    data->dblsptrmap.at(DBLVECT2)->insert(next(data->dblsptrmap.at(DBLVECT2)->begin(),idx),  target);
                }
                ++newdim;
            }
        }
        else {
            // replace at newdim
            data->posCounts.push_back(posCount);
            if (data->datatype == DTint32){
                int target;
                memcpy((void*)&target, memptr + 4, 4);
                data->intsptrmap.at(INTVECT1)->at(newdim) = target;
                memcpy((void*)&target, memptr + 8, 4);
                data->intsptrmap.at(INTVECT2)->at(newdim) = target;
            }
            else {
                double target;
                memcpy((void*)&target, memptr + 4, 8);
                data->dblsptrmap.at(DBLVECT1)->at(newdim) = target;
                memcpy((void*)&target, memptr + 12, 8);
                data->dblsptrmap.at(DBLVECT2)->at(newdim) = target;
            }
            ++newdim;
            highestPosCount = posCount;
        }
    }
    if (negativeError)
        cout << "Warning: Skipped values with negative PosCounter" << endl;
    if (doublePCError)
        cout << "Warning: PosCounter not unique; applied double poscounter workaround" << endl;

    if (data->datatype == DTint32){
        data->intsptrmap.at(INTVECT1)->resize(newdim);
        data->intsptrmap.at(INTVECT2)->resize(newdim);
    }
    else {
        data->dblsptrmap.at(DBLVECT1)->resize(newdim);
        data->dblsptrmap.at(DBLVECT2)->resize(newdim);
    }
    free(memBuffer);

}

void IH5File::addExtensionData(IData* data){

    string datasetname = data->getH5name();
    vector<int> exclusions;
    if (data->getNormalizeId().size() > 0)
        datasetname = data->getId();
    else {
        // since the name of averagemetadata is ambigous in all EVEH5 versions up to 4.0, use normalized data if any
        for (vector<IMetaData *>::iterator it = chainmeta.begin(); it != chainmeta.end(); ++it){
            IMetaData* mdat = *it;
            if ((mdat->getId().compare(data->getId()) == 0) && (mdat->getNormalizeId().size() > 0)){
                IData* normdata = new IData(*mdat);
                if (mdat->dstype == EVEDSTPCOneColumn) readDataPCOneCol(normdata);
                exclusions = normdata->getPosReferences();
                delete normdata;
                break;
            }
        }
        if (exclusions == data->getPosReferences()) return;
    }
    string fullh5name = data->getPath() + "averagemeta/" + datasetname + "__AverageCount";
    MetaData *extensionmd = findMetaData(extensionmeta, fullh5name);
    if (extensionmd != NULL){
        IData* avdata = new IData((IMetaData&)*extensionmd);
        readDataPCTwoCol(avdata);
        copyAndFill(avdata, DTint32, INTVECT1, data, DTint32, AVCOUNT, exclusions);
        copyAndFill(avdata, DTint32, INTVECT2, data, DTint32, AVATT, exclusions);
        delete avdata;
    }
    fullh5name = data->getPath() + "averagemeta/" + datasetname + "__Limit";
    extensionmd = findMetaData(extensionmeta, fullh5name);
    if (extensionmd != NULL){
        IData* avdata = new IData((IMetaData&)*extensionmd);
        readDataPCTwoCol(avdata);
        copyAndFill(avdata, DTfloat64, DBLVECT1, data, DTfloat64, AVLIMIT, exclusions);
        copyAndFill(avdata, DTfloat64, DBLVECT2, data, DTfloat64, AVMAXDEV, exclusions);
        delete avdata;
    }
    fullh5name = data->getPath() + "averagemeta/" + datasetname + "__MaxAttempts";
    extensionmd = findMetaData(extensionmeta, fullh5name);
    if (extensionmd != NULL){
        IData* avdata = new IData((IMetaData&)*extensionmd);
        readDataPCOneCol(avdata);
        copyAndFill(avdata, DTint32, INTVECT1, data, DTint32, AVATTPR, exclusions);
        delete avdata;
    }
    fullh5name = data->getPath() + "standarddev/" + datasetname + "__Count";
    extensionmd = findMetaData(extensionmeta, fullh5name);
    if (extensionmd != NULL){
        IData* avdata = new IData((IMetaData&)*extensionmd);
        readDataPCTwoCol(avdata);
        copyAndFill(avdata, DTfloat64, DBLVECT1, data, DTint32, STDDEVCOUNT, exclusions);
        copyAndFill(avdata, DTfloat64, DBLVECT2, data, DTfloat64, STDDEV, exclusions);
        delete avdata;
    }
}

void IH5File::copyAndFill(IData *srcdata, eve::DataType srctype, int srccol, IData *dstdata, eve::DataType dsttype, int dstcol, vector<int> excl){

    if (((srctype != DTint32) && (srctype != DTfloat64)) || ((dsttype != DTint32) && (dsttype != DTfloat64)))
        STHROW("unsupported datatype, currently only int32 or float64 are supported for data conversion (src)");

    vector<int>& srcPosCounts=srcdata->posCounts;
    vector<int>& dstPosCounts=dstdata->posCounts;

    if (srcPosCounts.size() == 0) return;

    if ((srctype == dsttype) && (srcPosCounts.size() == dstPosCounts.size()) && (srcPosCounts == dstPosCounts)){
        if (dsttype == DTint32){
            if (dstdata->intsptrmap.find(dstcol) != dstdata->intsptrmap.end()) dstdata->intsptrmap.erase(dstcol);
            dstdata->intsptrmap.insert(pair<int, shared_ptr<vector<int>>>(dstcol, srcdata->intsptrmap.at(srccol)));
        }
        else {
            if (dstdata->dblsptrmap.find(dstcol) != dstdata->dblsptrmap.end()) dstdata->dblsptrmap.erase(dstcol);
            dstdata->dblsptrmap.insert(pair<int, shared_ptr<vector<double>>>(dstcol, srcdata->dblsptrmap.at(srccol)));
        }
    }
    else {
        if (dsttype == DTint32) {
            if (dstdata->intsptrmap.find(dstcol) != dstdata->intsptrmap.end()) dstdata->intsptrmap.erase(dstcol);
            dstdata->intsptrmap.insert(pair<int, shared_ptr<vector<int>>>(dstcol, make_shared<vector<int>>(dstPosCounts.size())));
        }
        else {
            if (dstdata->dblsptrmap.find(dstcol) != dstdata->dblsptrmap.end()) dstdata->dblsptrmap.erase(dstcol);
            dstdata->dblsptrmap.insert(pair<int, shared_ptr<vector<double>>>(dstcol, make_shared<vector<double>>(dstPosCounts.size())));
        }

        unsigned int exclidx = 0;
        unsigned int exclsize = excl.size() ;
        unsigned int srcidx = 0;
        int srcposcountsize = srcPosCounts.size();
        unsigned int srcsize;
        bool copiedNone = true;
        if (srctype == DTint32)
            srcsize = srcdata->intsptrmap.at(srccol)->size();
        else
            srcsize = srcdata->dblsptrmap.at(srccol)->size();

        for (unsigned int i=0; i < dstPosCounts.size(); ++i){
            bool noexclude = true;
            int dstposcnt = dstPosCounts.at(i);
            while ((srcdata->posCounts[srcidx] < dstposcnt) && (srcidx < srcsize - 1)) ++ srcidx;
            if (exclsize > 0){
                while ((exclidx < (unsigned int) dstposcnt) && (exclidx < exclsize - 1)) ++exclidx;
                if (dstposcnt == excl.at(exclidx)) noexclude = false;
            }
            if ((srcposcountsize > 0) && (dstposcnt == srcdata->posCounts[srcidx]) && noexclude){
                if (srcdata->getDataType() == DTint32){
                    if (dsttype == DTint32)
                        dstdata->intsptrmap.at(dstcol)->at(i) = srcdata->intsptrmap.at(srccol)->at(srcidx);
                    else
                        dstdata->dblsptrmap.at(dstcol)->at(i) = (double)srcdata->intsptrmap.at(srccol)->at(srcidx);
                }
                else {
                    if (dsttype == DTint32) {
                        try {
                            dstdata->intsptrmap.at(dstcol)->at(i) = (int)srcdata->dblsptrmap.at(srccol)->at(srcidx);
                        } catch (Exception error) {
                            if (srcdata->dblsptrmap.at(srccol)->at(srcidx) > 0.0)
                                dstdata->intsptrmap.at(dstcol)->at(i) = INT_MAX;
                            else
                                dstdata->intsptrmap.at(dstcol)->at(i) = INT_MIN;
                        }
                    }
                    else {
                        dstdata->dblsptrmap.at(dstcol)->at(i) = srcdata->dblsptrmap.at(srccol)->at(srcidx);
                    }
                }
                ++srcidx;
                if (srcidx >= srcsize) --srcidx;
                copiedNone = false;
            }
            else {
                if (dsttype == DTint32)
                    dstdata->intsptrmap.at(dstcol)->at(i) = INT_MIN;
                else
                    dstdata->dblsptrmap.at(dstcol)->at(i) = NAN;
            }
        }
        if(copiedNone){
            if (dsttype == DTint32) {
                if (dstdata->intsptrmap.find(dstcol) != dstdata->intsptrmap.end()) dstdata->intsptrmap.erase(dstcol);
            }
            else {
                if (dstdata->dblsptrmap.find(dstcol) != dstdata->dblsptrmap.end()) dstdata->dblsptrmap.erase(dstcol);
            }
        }
    }
}

vector<string> IH5File::getLogData(){

    vector<string> stringlist;

    StrType tid1(0, H5T_VARIABLE);
//    hid_t		native_type;
    hsize_t dims;
    DataSet h5dset;

    try {
        h5dset = h5file.openDataSet("/LiveComment");
    }
    catch (Exception error) {
        return stringlist;
    }

    H5::DataType h5dtype = h5dset.getDataType();
    h5dset.getSpace().getSimpleExtentDims( &dims, NULL);

    /* Construct native type */
//    if((native_type=H5Tget_native_type(h5dtype.getId(), H5T_DIR_DEFAULT)) < 0 )
//        cerr << "get LiveComment: H5Tget_native_type  failed!!! " << endl;

    /* Check if the data type is equal */
//    if(!H5Tequal(native_type, tid1.getId()))
//        cerr << "get LiveComment: native type is not var length string!!!" << endl;

    char *rdata[dims];   /* Information read in */
    try {
        h5dset.read((void*)rdata, h5dtype);
    }
    catch (Exception error) {
        stringlist.push_back("error reading LiveComment");
        return stringlist;
    }
    /* Validate and print data read in */
    for(unsigned i=0; i<dims; i++) {
        stringlist.push_back(string(rdata[i]));
        free(rdata[i]);
    }

    return stringlist;
}

vector<Data*> IH5File::getJoinedData(vector<MetaData*>& mdvec, FillRule fillType){

    vector<IData*> datavect;
    vector<Data*> moddatavect;
    set<int> channelPosCounts;
    set<int> axisPosCounts;
    set<int> newlist;
    vector<int> posCounters;
    vector<MetaData*> standardList;
    vector<MetaData*> timeStampList;
    map<string, MetaData*> snapshotMap;

    if (mdvec.size() == 0) return moddatavect;

    for (MetaData* mdata: mdvec)
        if (mdata->getSection() == Standard)
            standardList.push_back(mdata);
        else if (mdata->getSection() == Timestamp)
            timeStampList.push_back(mdata);
        else if (mdata->getSection() == Snapshot)
            snapshotMap.insert(pair<string, MetaData*>(mdata->getId(), mdata));

    for (vector<MetaData*>::iterator mdit=standardList.begin(); mdit != standardList.end(); ++mdit){
        IData* idat = (IData*) getData((IMetaData*)*mdit);
        if (idat != NULL) datavect.push_back(idat);
        DeviceType deviceType = idat->getDeviceType();
        if (deviceType == Channel) {
            channelPosCounts.insert(idat->posCounts.begin(), idat->posCounts.end());
        }
        else if (deviceType == Axis) {
            axisPosCounts.insert(idat->posCounts.begin(), idat->posCounts.end());
        }

    }

    if (fillType == NoFill) {
        if ((channelPosCounts.size() == 0) || (axisPosCounts.size() == 0)) return moddatavect;
        for (set<int>::iterator it=axisPosCounts.begin(); it != axisPosCounts.end(); ++it){
            if (channelPosCounts.find(*it) != channelPosCounts.end()) newlist.insert(*it);
        }
    }
    else if (fillType == LastFill) {
        if (channelPosCounts.size() == 0) return moddatavect;
        newlist.insert(channelPosCounts.begin(), channelPosCounts.end());
    }
    else if (fillType == NANFill) {
        if (axisPosCounts.size() == 0) return moddatavect;
        newlist.insert(axisPosCounts.begin(), axisPosCounts.end());
    }
    else if (fillType == LastNANFill) {
        if ((axisPosCounts.size() == 0) && (channelPosCounts.size() == 0)) return moddatavect;
        newlist.insert(axisPosCounts.begin(), axisPosCounts.end());
        newlist.insert(channelPosCounts.begin(), channelPosCounts.end());
    }

    posCounters.insert(posCounters.begin(),newlist.begin(), newlist.end());
    // add timestamp here, because it is not used to calc posCounters
    for (vector<MetaData*>::iterator mdit=timeStampList.begin(); mdit != timeStampList.end(); ++mdit){
        IData* idat = (IData*) getData((IMetaData*)*mdit);
        if (idat != NULL) datavect.push_back(idat);
    }

    for (vector<IData*>::iterator datait=datavect.begin(); datait != datavect.end(); ++datait){
        IData* newData = *datait;
        if ((newData->posCounts.size() != posCounters.size()) || (newData->posCounts != posCounters)) {
            if (((fillType == LastFill) || (fillType == LastNANFill))
                    && (newData->getDeviceType() == Axis)
                    && snapshotMap.find(newData->getId()) != snapshotMap.end()){
                // need to fill in start value from snapshot
                IData* snapData = (IData*) getData((IMetaData*)snapshotMap.find(newData->getId())->second);
                newData = new IData(*newData, posCounters, fillType, snapData);
            }
            else {
                newData = new IData(*newData, posCounters, fillType);
            }
            delete *datait;
        }
        moddatavect.push_back(newData);
    }
    return moddatavect;
}

vector<Data*> IH5File::getPreferredData(FillRule fill){
    string prefAxis = "";
    string prefChannel = "";
    vector<MetaData*> mdvect;

    map<string, string>::iterator it = chainAttributes.find("preferredAxis");
    if (it != chainAttributes.end()) prefAxis = it->second;
    it = chainAttributes.find("preferredChannel");
    if (it != chainAttributes.end()) prefChannel = it->second;
    it = chainAttributes.find("PreferredNormalizationChannel");
    if (it != chainAttributes.end()) prefChannel = "normalized/" + prefChannel + "__" + it->second;

    if ((prefAxis.size() > 0) && (prefChannel.size() > 0)){
        prefAxis = getSectionString(Standard) + "/" + prefAxis;
        MetaData* axismd = findMetaData(chainmeta, prefAxis);
        prefChannel = getSectionString(Standard) + "/" + prefChannel;
        MetaData* channelmd = findMetaData(chainmeta, prefChannel);
        if ((axismd != NULL) && (channelmd != NULL)) {
            mdvect.push_back(axismd);
            mdvect.push_back(channelmd);
        }
    }
    return getJoinedData(mdvect, fill);
}

vector<Data*> IH5File::getData(vector<MetaData*>& md){
    vector<Data*> datavect;

    for (vector<MetaData*>::iterator mdit=md.begin(); mdit != md.end(); ++mdit){
        Data* idat = getData((IMetaData*)*mdit);
        if (idat != NULL) datavect.push_back(idat);
    }
    return datavect;
}

} // namespace end
