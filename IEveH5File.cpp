#include <string.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <set>
#include "IEveH5File.h"
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

#define EVEH5VERSIONMAXIMUM 4.99
#define EVEH5VERSIONMINIMUM 3.0

EveH5File* EveH5File::openH5File(string name){return new IEveH5File(name);};

IEveH5File::IEveH5File(string filename)
{

    isOpen = false;
    name2xmlid.clear();
    xmlidMap.clear();
    monitorxmlidMap.clear();
    chainList.clear();
    rootAttributes.clear();
//    vector<string> storageTypes = {"normalized","center","edge","minimum","maximum","fwhm","mean","standarddev","sum","peak","device","meta","averagemeta","unknown"};
//    vector<string> storagePlaces(string("main"),string("pre_post"), string("meta"));
    open(filename);
}

IEveH5File::~IEveH5File()
{
    close();
}

void IEveH5File::open(string filename)
{

    Exception::dontPrint();

    try {
        if (!H5File::isHdf5(filename)) return;
        h5file.openFile(filename, H5F_ACC_RDONLY);
    }
    catch (Exception error){
        STHROW("Error opening file " << filename << "; H5 Error: " << error.getDetailMsg() );
    }


    Group root;
    openGroup(root, "/");
    checkVersion(root) ;
    getAttributes(root, rootAttributes);
    chainList = getGroups(root, "device");

    // walk through all chains
    for (vector<string>::iterator cit=chainList.begin(); cit != chainList.end(); ++cit){
        Group chain;
        openGroup(chain, *cit);
        vector<string> pathlist = getGroups(chain, "");
        closeGroup(chain);

        // walk through all storagePlaces (meta, main, pre_post)
        for (vector<string>::iterator pit=pathlist.begin(); pit != pathlist.end(); ++pit){
            string path="/"+*cit+"/"+*pit;
            Group pathgr;
            openGroup(pathgr, path);
            parseDatasets(pathgr, path, &xmlidMap, &name2xmlid);
            parseGroupDatasets(pathgr, path, &xmlidMap, &name2xmlid);

            // walk through all non-raw data (normalized standarddev etc.)
            vector<string> calclist = getGroups(pathgr, "");
            for (vector<string>::iterator calcit=calclist.begin(); calcit != calclist.end(); ++calcit){
                string calcpath = path + "/" + *calcit;
                Group calcgr;
                openGroup(calcgr, calcpath);
                parseDatasets(calcgr, calcpath, &xmlidMap, &name2xmlid);
                parseGroupDatasets(calcgr, calcpath, &xmlidMap, &name2xmlid);
                closeGroup(calcgr);
            }
            closeGroup(pathgr);
        }
    }
    if (haveGroupWithName(root, "device")){
        Group devices;
        openGroup(devices, "/device");
        parseDatasets(devices, "/device", &monitorxmlidMap, &name2xmlid);
        closeGroup(devices);
    }
    closeGroup(root);
    isOpen = true;
    return;
}

multimap<string, string> IEveH5File::getChainAttributes(string chainname){

    bool found = false;
    multimap<string, string> chainAttributes;

    for (vector<string>::iterator cit=chainList.begin(); cit != chainList.end(); ++cit){
        if (*cit == chainname) found = true;
    }

    if (found) {
        Group chain;
        openGroup(chain, "/"+chainname);
        getAttributes(chain, chainAttributes);
    }
    return chainAttributes;
}

void IEveH5File::openGroup(Group& h5group, string path){
    try {
        h5group = h5file.openGroup(path);
    }
    catch (Exception error){
        STHROW("Error opening group " << path << "; H5 Error: " << error.getDetailMsg() );
    }
}

void IEveH5File::closeGroup(Group& h5group){
    try {
        h5group.close();
    }
    catch (Exception error){
        STHROW("Error closing group; H5 Error: " << error.getDetailMsg() );
    }
}

void IEveH5File::checkVersion(Group& rootgroup){

    Attribute versionattr;
    try {
        versionattr = rootgroup.openAttribute("EVEH5Version");
    }
    catch (Exception error){
        STHROW("No EVEH5 version information available; H5 Error: " << error.getDetailMsg() );
    }
    StrType stread = StrType(versionattr.getStrType());
    string versionString;
    versionattr.read(stread, versionString);
    float versionf = strtof(versionString.c_str(), NULL);
    if ((versionf > EVEH5VERSIONMAXIMUM) || (versionf < EVEH5VERSIONMINIMUM))
        STHROW("EVEH5 version mismatch, file " << versionf << ", supported "
               << (float)EVEH5VERSIONMINIMUM << " - " << (float)EVEH5VERSIONMAXIMUM << "\n");
    return;
}

// collect all groups with array data i.e. with attribute "XML-ID"
void IEveH5File::parseGroupDatasets(Group& group, string prefix, map<string, IEveDataInfo*> *idmap, multimap<string, string> *nameidmap)
{

    vector<string> dsgroups = getGroups(group, "");
    for (vector<string>::iterator it=dsgroups.begin(); it != dsgroups.end(); ++it){

        Group dsgroup;
        string fullname = prefix + "/" + *it;
        openGroup(dsgroup, fullname);

        IEveDataInfo* dinfo = new IEveDataInfo();
        getAttributes(dsgroup, dinfo->attributes);

        if (dinfo->attributes.count("XML-ID") > 0)
            dinfo->xmlId = dinfo->attributes.find("XML-ID")->second;

        if (!dinfo->xmlId.empty()){
            dinfo->h5name = *it;
            dinfo->path = prefix + "/";
            dinfo->calculation = EVEraw;
            idmap->insert ( pair<string, IEveDataInfo*>(fullname, dinfo) );
            if (dinfo->attributes.count("Name") > 0) {
                dinfo->name = dinfo->attributes.find("Name")->second;
            }
            if (!dinfo->name.empty())
                nameidmap->insert(pair<string, string>(dinfo->name, dinfo->xmlId));
            if (dinfo->attributes.count("DeviceType") > 0) {
                string devtypestr = dinfo->attributes.find("DeviceType")->second;
                if (devtypestr.compare("Axis") == 0)
                    dinfo->devtype = DEVTAxis;
                else if (devtypestr.compare("Channel") == 0)
                    dinfo->devtype = DEVTChannel;
            }
            dinfo->dstype = EVEDSTArray;
//            dsList.push_back(fullname);
            for (hsize_t index = 0; index < dsgroup.getNumObjs(); ++index){
                if (dsgroup.getObjTypeByIdx(index) != H5G_DATASET) continue;
                string subname = dsgroup.getObjnameByIdx(index);
                int posCnt = strtol(subname.c_str(),NULL,10);
                dinfo->posCounts.push_back(posCnt);
                // open the first dataset to retrieve the datatype
                if (index  == 0) {
                    DataSet ds = dsgroup.openDataSet(subname);
                    dinfo->setDataType(ds);
                    ds.close();
                }
                ++dinfo->dim0;
            }
        }
        else {
            // TODO unknown group
            delete dinfo;
        }
        closeGroup(dsgroup);
    }
}

void IEveH5File::parseDatasets(Group& group, string prefix, map<string, IEveDataInfo*> *idmap, multimap<string, string> *nameidmap){
    for (hsize_t index = 0; index < group.getNumObjs(); ++index){
        string objname = group.getObjnameByIdx(index);
        string fullname = prefix + "/" + objname;
        if (group.getObjTypeByIdx(index) == H5G_DATASET){
            DataSet ds = group.openDataSet(objname);
            IEveDataInfo* dinfo = new IEveDataInfo();
            dinfo->calculation = EVEraw;
            getAttributes(ds, dinfo->attributes);
            dinfo->h5name = objname;
            dinfo->path = prefix + "/";
            if (dinfo->attributes.count("axis") > 0)
                dinfo->xmlId = dinfo->attributes.find("axis")->second;
            // overwrites xmlId if present
            if (dinfo->attributes.count("XML-ID") > 0)
                dinfo->xmlId = dinfo->attributes.find("XML-ID")->second;
            if (dinfo->attributes.count("channel") > 0)
                dinfo->channelId = dinfo->attributes.find("channel")->second;
            if (dinfo->attributes.count("normalizeId") > 0)
                dinfo->normalizeId = dinfo->attributes.find("normalizeId")->second;
            if (dinfo->attributes.count("Name") > 0)
                dinfo->name = dinfo->attributes.find("Name")->second;

            // device data
            if (dinfo->xmlId.empty()) dinfo->xmlId = objname;


            idmap->insert ( pair<string, IEveDataInfo*>(fullname, dinfo) );

            if (!dinfo->name.empty() && !dinfo->xmlId.empty())
                nameidmap->insert(pair<string, string>(dinfo->name, dinfo->xmlId));
            if (dinfo->attributes.count("DeviceType") > 0) {
                string devtypestr = dinfo->attributes.find("DeviceType")->second;
                if (devtypestr.compare("Axis") == 0)
                    dinfo->devtype = DEVTAxis;
                else if (devtypestr.compare("Channel") == 0)
                    dinfo->devtype = DEVTChannel;
            }
            dinfo->setDataType(ds);
//            list.push_back(fullname);
            ds.close();
        }
    }
}

void IEveH5File::close()
{
    isOpen = false;
    name2xmlid.clear();
    xmlidMap.clear();
    monitorxmlidMap.clear();
    chainList.clear();
//    dsList.clear();
    rootAttributes.clear();
    try {
        h5file.close();
    }
    catch (Exception error){
        STHROW("Error closing file; H5 Error: " << error.getDetailMsg() );
    }
}

// return the names of all groups in group, except if name is ignore
vector<string> IEveH5File::getGroups(Group& group, string ignore){
    vector<string> groupList;
    for (hsize_t index = 0; index < group.getNumObjs(); ++index){
        string objname = group.getObjnameByIdx(index);
        if (group.getObjTypeByIdx(index) == H5G_GROUP){
            if (objname.compare(ignore) != 0)
                groupList.push_back(objname);
        }
    }
    return groupList;
}

bool IEveH5File::haveGroupWithName(Group& group, string name){
    vector<string> grouplist = getGroups(group, "");
    for (vector<string>::iterator it=grouplist.begin(); it != grouplist.end(); ++it){
        if ((*it).compare(name) == 0) return true;
    }
    return false;
}

vector<string> IEveH5File::getPathsForId(string dataId){
    vector<string> fullPaths;
    for (map<string, IEveDataInfo*>::iterator it=xmlidMap.begin(); it!=xmlidMap.end(); ++it){
        if (it->second->xmlId.compare(dataId) == 0) fullPaths.push_back(it->first);
    }
    return fullPaths;
}

IEveDataInfo* IEveH5File::getDataInfoForPath(string path){
    map<string, IEveDataInfo*>::iterator it = xmlidMap.find(path);
    if (it!=xmlidMap.end())
        return new IEveDataInfo(*(it->second));
    else
        return NULL;
}

int IEveH5File::getAttributes(H5Location& object, multimap<string, string>& attribMap){

    unsigned int numAttrib = object.getNumAttrs();
    std::multimap<string,string>::iterator it;
    for (unsigned int index = 0; index < numAttrib; ++index){
        Attribute attrib = object.openAttribute(index);
        string name = attrib.getName();
        StrType stread = StrType(attrib.getStrType());
        int length = stread.getSize();
        char *strg_C = new char[(size_t)length+1];
        attrib.read(stread, strg_C);
        strg_C[length] = 0;
        it=attribMap.insert ( pair<string, string>(name, string(strg_C)) );
        delete[] strg_C;
        attrib.close();
    }
    return numAttrib;
}

//list<string> IEveH5File::getDatasetIds(string chain, EVECalc modified){
//    list<string> datasets;
//    if (modified == EVEraw){
//        string fullname = "/" + chain + "/";
//         for (std::list<string>::iterator it=dsList.begin(); it != dsList.end(); ++it){
//             if (it->compare(0, fullname.length(), fullname) == 0) {
//                 datasets.push_back(*it);
//             }
//         }
//    }
//    else {
//        string fullname = "/" + chain + "/" + IEveDataInfo::toCalcString(modified) +"/";
//         for (std::list<string>::iterator it=modDsList.begin(); it != modDsList.end(); ++it){
//             if (it->compare(0, fullname.length(), fullname) == 0) {
//                 datasets.push_back(*it);
//             }
//         }
//    }
//    return datasets;
//}

//vector<string> IEveH5File::getCalcNames(string chain){
//    vector<string> datasets;
//    pair<multimap<string,string>::iterator, multimap<string,string>::iterator> rangeit;
//    rangeit = chain2Modified.equal_range("/" + chain);
//    for (multimap<string,string>::iterator it=rangeit.first; it!=rangeit.second; ++it)
//         datasets.push_back(it->second);
//    return datasets;
//}

vector<string> IEveH5File::getDeviceNames(){
    vector<string> devnames;
    for (multimap<string,string>::iterator it=name2xmlid.begin(); it!=name2xmlid.end(); ++it)
         devnames.push_back(it->first);
    return devnames;
}

vector<string> IEveH5File::getDeviceIds(){
    vector<string> output;
    set<string> devIds;
    for (map<string, IEveDataInfo*>::iterator it=xmlidMap.begin(); it!=xmlidMap.end(); ++it){
        if (!it->second->xmlId.empty()) {
            devIds.insert(it->second->xmlId);
        }
    }
    for (set<string>::iterator it=devIds.begin(); it!=devIds.end(); ++it)
        output.push_back(*it);

    return output;
}

vector<string> IEveH5File::getMonitorDeviceIds(){
    vector<string> devIds;
    for (map<string, IEveDataInfo*>::iterator it=monitorxmlidMap.begin(); it!=monitorxmlidMap.end(); ++it){
        if (!it->second->xmlId.empty()) devIds.push_back(it->second->xmlId);
    }
    return devIds;
}

vector<string> IEveH5File::getDeviceIdForName(string name){
    vector<string> devnames;
    if (name.length() > 0){
        pair<multimap<string,string>::iterator, multimap<string,string>::iterator> rangeit;
        rangeit = name2xmlid.equal_range(name);
        for (multimap<string,string>::iterator it=rangeit.first; it!=rangeit.second; ++it)
            devnames.push_back(it->second);
    }
    return devnames;
}


IEveData* IEveH5File::getData(EveDataInfo* dInfo){

    return getDataSkipVerify((IEveDataInfo*)dInfo);
}

IEveData* IEveH5File::getDataSkipVerify(IEveDataInfo* dInfo){

    // Note: if valueCount==0 we must have a valid (empty) container
    IEveData* dataObj=NULL;
    hsize_t dims_out[2];
    size_t element_size;

    if (dInfo->dstype == EVEDSTArray){
        if ((dInfo->datatype != DTint32) && (dInfo->datatype != DTfloat64))
            STHROW("unsupported datatype, currently only int32 or float64 are supported for arrays");

        dataObj = new IEveData(*dInfo);
        for (vector<int>::iterator it = dInfo->posCounts.begin(); it != dInfo->posCounts.end(); ++it){
            ostringstream fullname;
            DataSet h5dset;
            DataType h5dtype;

            fullname << dInfo->path << dInfo->h5name << "/" << *it;
            string objname = fullname.str();

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
            if ((dims_out[1] > 1) || (dInfo->h5dimensions[0] != dims_out[0]) || (dInfo->h5dimensions[1] != dims_out[1]))
                STHROW("Error: Unexpected dimension in Dataset " << objname);
            if ((dInfo->datatype == DTint32) && (element_size != sizeof(int)))
                STHROW("Error: Unexpected integer size in Dataset " << objname);
            if ((dInfo->datatype == DTfloat64) && (element_size != sizeof(double)))
                STHROW("Error: Unexpected float size in Dataset " << objname);

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
            dataObj->posPtrHash.insert(pair<int, void*>(*it, memBuffer));
        }
    }
    else if (dInfo->dstype == EVEDSTPCOneColumn){

        if ((dInfo->datatype == DTunknown)) STHROW("unsupported datatype");

        DataSet h5dset;
        DataType h5dtype;
        string objname = dInfo->path + dInfo->h5name;

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
        if (dInfo->h5dimensions[0] != dims_out[0])
            STHROW("Unsupported dimension error in Dataset " << objname);

        switch (dInfo->datatype) {
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

        dataObj = new IEveData(*dInfo);
        bool typeerror = false;
        char *memptr = (char*)memBuffer;
        for (unsigned int i = 0; i < dims_out[0]; ++i){
            dataObj->posCounts.push_back(*((int*)memptr));
            if (dInfo->datatype == DTint32) {
                dataObj->intvect.push_back(*((int*)(memptr + 4)));
            }
            else if (dInfo->datatype == DTuint32) {
                dataObj->intvect.push_back((int)*((unsigned int*)(memptr + 4)));
            }
            else if (dInfo->datatype == DTint8) {
                dataObj->intvect.push_back((int)*(memptr + 4));
            }
            else if (dInfo->datatype == DTuint8) {
                dataObj->intvect.push_back((int)*((unsigned char*)(memptr + 4)));
            }
            else if (dInfo->datatype == DTint16) {
                dataObj->intvect.push_back((int)*((short*)(memptr + 4)));
            }
            else if (dInfo->datatype == DTuint16) {
                dataObj->intvect.push_back((int)*((unsigned short*)(memptr + 4)));
            }
            else if (dInfo->datatype == DTfloat32) {
                dataObj->dblvect.push_back((double)*((float*)(memptr + 4)));
            }
            else if (dInfo->datatype == DTfloat64) {
                dataObj->dblvect.push_back(*((double*)(memptr + 4)));
            }
            else if ((dInfo->datatype == DTstring)) {
                string tmpstring(memptr + 4, element_size-4);
                dataObj->strvect.push_back(tmpstring);
            }
            else
                typeerror = true;

            memptr += element_size;
        }
        free(memBuffer);
        if (typeerror) STHROW("Unable to read data: unknown DataSet type");
    }
    else if (dInfo->dstype == EVEDSTPCTwoColumn){
        if ((dInfo->datatype != DTint32) && (dInfo->datatype != DTfloat64))
            STHROW("unsupported datatype, currently only int32 and float64 is supported for two column arrays");

        DataSet h5dset;
        DataType h5dtype;
        string objname = dInfo->path + dInfo->h5name;

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
        if (dInfo->h5dimensions[0] != dims_out[0])
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

        dataObj = new IEveData(*dInfo);
        char *memptr = (char*)memBuffer;
        unsigned int size=8;
        if (dInfo->datatype == DTfloat64) size = 16;

        for (unsigned int i = 0; i < dims_out[0]; ++i){
            int posCount=*((int*)memptr);
            dataObj->posCounts.push_back(posCount);
            void *dataPtr = malloc(size);
            if (dataPtr == NULL)
                STHROW("Unable to allocate memory while reading DataSet: " << objname);
            memcpy(dataPtr, memptr + 4, size);
            dataObj->posPtrHash.insert(pair<int, void*>(posCount, (void*) dataPtr));
            memptr += element_size;
        }
        free(memBuffer);
    }
    else {
        STHROW("Unable to read data: unknown DataSet type");
    }
    return dataObj;
}

IEveDataInfo* IEveH5File::getMonitorDataInfo(string xmlid){
    map<string, IEveDataInfo*>::iterator it = monitorxmlidMap.find("/device/" + xmlid);
    if (it!=monitorxmlidMap.end()){
        IEveDataInfo *dInfo = new IEveDataInfo(*(it->second));
        dInfo->dstype = EVEDSTPCOneColumn;
        dInfo->isMonitor = true;
        return dInfo;
    }
    else
        return NULL;
}
