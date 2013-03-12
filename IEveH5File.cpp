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

EveH5File* EveH5File::openH5File(string name){return new IEveH5File(name);};

IEveH5File::IEveH5File(string filename)
{
    isOpen = false;
    haveDeviceGroup = false;
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
    try {
        root = h5file.openGroup("/");
    }
    catch (Exception error){
        STHROW("Error opening root group; H5 Error: " << error.getDetailMsg() );
    }
    getAttributes(root, rootAttributes);
    getRootGroups(root);
    try {
    root.close();
    }
    catch (Exception error){
        STHROW("Error closing root group; H5 Error: " << error.getDetailMsg() );
    }

    for (vector<string>::iterator it=chainList.begin(); it != chainList.end(); ++it){

        Group chain;
        try {
            chain = h5file.openGroup(*it);
        }
        catch (Exception error){
            STHROW("Error opening group " << *it << "; H5 Error: " << error.getDetailMsg() );
        }
        parseDatasets(chain, "/" + *it, EVEraw, dsList);
        parseGroups(chain, "/" + *it);

        try {
        chain.close();
        }
        catch (Exception error){
            STHROW("Error closing group " << *it << "; H5 Error: " << error.getDetailMsg() );
        }
    }
    if (haveDeviceGroup){
        Group devices;
        try {
            devices = h5file.openGroup("/device");
        }
        catch (Exception error){
            STHROW("Error opening group device; H5 Error: " << error.getDetailMsg() );
        }
        parseDatasets(devices, "/device", EVEraw, deviceDsList);
        try {
            devices.close();
        }
        catch (Exception error){
            STHROW("Error closing group device; H5 Error: " << error.getDetailMsg() );
        }
    }
    isOpen = true;
    return;
}

void IEveH5File::parseGroups(Group& group, string prefix){
    for (hsize_t index = 0; index < group.getNumObjs(); ++index){
        string objname = group.getObjnameByIdx(index);
        string fullname = prefix + "/" + objname;
        EVECalc calculation = EVEraw;
        if (group.getObjTypeByIdx(index) == H5G_GROUP){
            if (objname.compare("maximum") == 0)
                calculation = EVEmaximum;
            else if (objname.compare("minimum") == 0)
                calculation = EVEminimum;
            else if (objname.compare("center") == 0)
                calculation = EVEcenter;
            else if (objname.compare("edge") == 0)
                calculation = EVEedge;
            else if (objname.compare("fwhm") == 0)
                calculation = EVEfwhm;
            else if (objname.compare("mean") == 0)
                calculation = EVEmean;
            else if (objname.compare("normalized") == 0)
                calculation = EVEnormalized;
            else if (objname.compare("standarddev") == 0)
                calculation = EVEstddev;
            else if (objname.compare("meta") == 0)
                calculation = EVEmeta;
            else if (objname.compare("sum") == 0)
                calculation = EVEsum;
            else if (objname.compare("peak") == 0)
                calculation = EVEpeak;

            Group subgroup;
            try {
                subgroup = group.openGroup(objname);
            }
            catch (Exception error){
                STHROW("Error opening group " << objname << "; H5 Error: " << error.getDetailMsg() );
            }

            if (calculation == EVEraw){
                IEveDataInfo* dinfo = new IEveDataInfo();
                getAttributes(subgroup, dinfo->attributes);

                if (dinfo->attributes.count("XML-ID") > 0)
                    dinfo->xmlId = dinfo->attributes.find("XML-ID")->second;

                if (!dinfo->xmlId.empty()){
                    dinfo->h5name = objname;
                    dinfo->path = prefix + "/";
                    dinfo->calculation = calculation;
                    xmlidMap.insert ( pair<string, IEveDataInfo*>(fullname, dinfo) );
                    if (!dinfo->name.empty())
                        name2xmlid.insert(pair<string, string>(dinfo->name, dinfo->xmlId));
                    if (dinfo->attributes.count("Name") > 0) {
                        dinfo->name = dinfo->attributes.find("Name")->second;
                    }
                    if (dinfo->attributes.count("DeviceType") > 0) {
                        string devtypestr = dinfo->attributes.find("DeviceType")->second;
                        if (devtypestr.compare("Axis") == 0)
                            dinfo->devtype = DEVTAxis;
                        else if (devtypestr.compare("Channel") == 0)
                            dinfo->devtype = DEVTChannel;
                    }
                    dinfo->dstype = EVEDSTArray;
                    dsList.push_back(fullname);
                    for (hsize_t index2 = 0; index2 < subgroup.getNumObjs(); ++index2){
                        string subname = subgroup.getObjnameByIdx(index2);
                        int posCnt = strtol(subname.c_str(),NULL,10);
                        dinfo->posCounts.push_back(posCnt);
                        // open the first dataset to retrieve the datatype
                        if (index2  == 0) {
                            DataSet ds = subgroup.openDataSet(subname);
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
            }
            else {
                chain2Modified.insert(pair<string, string>(prefix, IEveDataInfo::toCalcString(calculation)));
                parseDatasets(subgroup, fullname, calculation, modDsList);
            }
            try {
                subgroup.close();
            }
            catch (Exception error){
                STHROW("Error closing group " << objname << "; H5 Error: " << error.getDetailMsg() );
            }

        }
    }
}

void IEveH5File::parseDatasets(Group& group, string prefix, EVECalc mtype, list<string>& list){
    for (hsize_t index = 0; index < group.getNumObjs(); ++index){
        string objname = group.getObjnameByIdx(index);
        string fullname = prefix + "/" + objname;
        if (group.getObjTypeByIdx(index) == H5G_DATASET){
            DataSet ds = group.openDataSet(objname);
            IEveDataInfo* dinfo = new IEveDataInfo();
            dinfo->calculation = mtype;
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

            xmlidMap.insert ( pair<string, IEveDataInfo*>(fullname, dinfo) );
            if (!dinfo->name.empty() && !dinfo->xmlId.empty())
                name2xmlid.insert(pair<string, string>(dinfo->name, dinfo->xmlId));
            if (dinfo->attributes.count("DeviceType") > 0) {
                string devtypestr = dinfo->attributes.find("DeviceType")->second;
                if (devtypestr.compare("Axis") == 0)
                    dinfo->devtype = DEVTAxis;
                else if (devtypestr.compare("Channel") == 0)
                    dinfo->devtype = DEVTChannel;
            }
            dinfo->setDataType(ds);
            list.push_back(fullname);
            ds.close();
        }
    }
}

void IEveH5File::close()
{
    isOpen = false;
    name2xmlid.clear();
    xmlidMap.clear();
    chainList.clear();
    dsList.clear();
    modDsList.clear();
    rootAttributes.clear();
    try {
        h5file.close();
    }
    catch (Exception error){
        STHROW("Error closing file; H5 Error: " << error.getDetailMsg() );
    }
}


void IEveH5File::getRootGroups(Group& group){
    for (hsize_t index = 0; index < group.getNumObjs(); ++index){
        string objname = group.getObjnameByIdx(index);
        if (group.getObjTypeByIdx(index) == H5G_GROUP){
            if (objname.compare("device") == 0)
                haveDeviceGroup = true;
            else
                chainList.push_back(objname);
        }
    }
}

int IEveH5File::getAttributes(H5Object& object, multimap<string, string>& attribMap){

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

list<string> IEveH5File::getDatasetIds(string chain, EVECalc modified){
    list<string> datasets;
    if (modified == EVEraw){
        string fullname = "/" + chain + "/";
         for (std::list<string>::iterator it=dsList.begin(); it != dsList.end(); ++it){
             if (it->compare(0, fullname.length(), fullname) == 0) {
                 datasets.push_back(*it);
             }
         }
    }
    else {
        string fullname = "/" + chain + "/" + IEveDataInfo::toCalcString(modified) +"/";
         for (std::list<string>::iterator it=modDsList.begin(); it != modDsList.end(); ++it){
             if (it->compare(0, fullname.length(), fullname) == 0) {
                 datasets.push_back(*it);
             }
         }
    }
    return datasets;
}

vector<string> IEveH5File::getCalcNames(string chain){
    vector<string> datasets;
    pair<multimap<string,string>::iterator, multimap<string,string>::iterator> rangeit;
    rangeit = chain2Modified.equal_range("/" + chain);
    for (multimap<string,string>::iterator it=rangeit.first; it!=rangeit.second; ++it)
         datasets.push_back(it->second);
    return datasets;
}

vector<string> IEveH5File::getDeviceNames(){
    vector<string> devnames;
    for (multimap<string,string>::iterator it=name2xmlid.begin(); it!=name2xmlid.end(); ++it)
         devnames.push_back(it->first);
    return devnames;
}

vector<string> IEveH5File::getDeviceIds(){
    vector<string> devIds;
    set<string> deviceIdSet;
    for (map<string, IEveDataInfo*>::iterator it=xmlidMap.begin(); it!=xmlidMap.end(); ++it){
        if (!it->second->xmlId.empty()) deviceIdSet.insert(it->second->xmlId);
    }
    for (set<string>::iterator it=deviceIdSet.begin(); it!=deviceIdSet.end(); ++it)
        devIds.push_back(*it);
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

IEveDataInfo* IEveH5File::getDataInfo(string chain, string xmlid){

    map<string, IEveDataInfo*>::iterator it = xmlidMap.find("/" + chain + "/" + xmlid);
    if (it!=xmlidMap.end())
        return new IEveDataInfo(*(it->second));
    else
        return NULL;
}

IEveDataInfo* IEveH5File::getDataInfoByName(string chain, string name){

    vector<string> devIds = getDeviceIdForName(name);
    if (devIds.size() > 0) {
        // possibly ambigous
        return getDataInfo(chain, devIds.front());
    }
    return NULL;
}

IEveDataInfo* IEveH5File::getDataInfo(string chain, string axisId, string channelId, string normalizeId, string calculation){

    if (haveCalculation(chain, calculation)){
        EVECalc calcType = IEveDataInfo::toCalcType(calculation);
        for (map<string, IEveDataInfo*>::iterator it=xmlidMap.begin(); it!=xmlidMap.end(); ++it){
            if ((axisId.compare(it->second->xmlId) == 0) && (channelId.compare(it->second->channelId) == 0)
                    && (normalizeId.compare(it->second->normalizeId) == 0)
                    && (it->second->path.compare(1, chain.length(), chain)==0)
                    && (it->second->calculation == calcType)){
                return new IEveDataInfo(*(it->second));
            }
        }
    }
    return NULL;
}

IEveDataInfo* IEveH5File::getDataInfoByName(string chain, string axis, string channel, string normalize, string calculation){

    vector<string> axislist=getDeviceIdForName(axis);
    vector<string> channellist = getDeviceIdForName(channel);
    vector<string> normalizelist = getDeviceIdForName(normalize);
    string axisId;
    string channelId;
    string normalizeId;

    if (axislist.size() > 0)
        axisId = axislist.front();
    if (channellist.size() > 0)
        channelId = channellist.front();
    if (normalizelist.size() > 0)
        normalizeId = normalizelist.front();

    return getDataInfo(chain, axis, channel, normalize, calculation);
}

bool IEveH5File::haveCalculation(string chain, string calculation){
    pair <multimap<string, string>::iterator, multimap<string, string>::iterator> rangeIt;
    rangeIt=chain2Modified.equal_range("/" + chain);
    for (multimap<string, string>::iterator it=rangeIt.first; it!=rangeIt.second; ++it){
        if (it!=chain2Modified.end() && (calculation.compare(it->second) == 0)) return true;
    }
    return false;
}

IEveData* IEveH5File::getData(EveDataInfo* dInfo){

    // TODO the dInfo object might have been copied, so we connot compare address to
    // verify that it is in the list.
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

        if ((dInfo->datatype != DTint32) && (dInfo->datatype != DTfloat64) && (dInfo->datatype != DTstring))
            STHROW("unsupported datatype, currently only int32, float64 and strings are supported");

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
        if ((dInfo->datatype == DTint32) && (element_size != sizeof(intstruct)))
            STHROW("Unsupported integer size error in Dataset " << objname);
        if ((dInfo->datatype == DTfloat64) && (element_size != sizeof(dblstruct)))
            STHROW("Unsupported float size error in Dataset " << objname);
        if ((dInfo->datatype == DTstring) && ((element_size != STR_STRUCT_SIZE) && (element_size != ENUM_STRUCT_SIZE)))
            STHROW("Unsupported string size error in Dataset " << objname);

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
        for (unsigned int i = 0; i < dims_out[0]; ++i){
            if (dInfo->datatype == DTint32) {
                dataObj->posCounts.push_back(((struct intstruct*)memBuffer)[i].posCount);
                dataObj->intvect.push_back(((struct intstruct*)memBuffer)[i].data);
            }
            else if (dInfo->datatype == DTfloat64) {
                dataObj->posCounts.push_back(((struct dblstruct*)memBuffer)[i].posCount);
                dataObj->dblvect.push_back(((struct dblstruct*)memBuffer)[i].data);
            }
            else if ((dInfo->datatype == DTstring) && (element_size == STR_STRUCT_SIZE)) {
                dataObj->posCounts.push_back(((struct strstruct*)memBuffer)[i].posCount);
                dataObj->strvect.push_back(((struct strstruct*)memBuffer)[i].data);
            }
            else if ((dInfo->datatype == DTstring) && (element_size == ENUM_STRUCT_SIZE)) {
                dataObj->posCounts.push_back(((struct enumstruct*)memBuffer)[i].posCount);
                dataObj->strvect.push_back(((struct enumstruct*)memBuffer)[i].data);
            }
            else
                typeerror = true;
        }
        free(memBuffer);
        if (typeerror) STHROW("Unable to read data: unknown DataSet type");
    }
    else if (dInfo->dstype == EVEDSTPCTwoColumn){
        if (dInfo->datatype != DTfloat64)
            STHROW("unsupported datatype, currently only float64 is supported for two column arrays");

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
        if (element_size != sizeof(twodblstruct))
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
        for (unsigned int i = 0; i < dims_out[0]; ++i){
            double* dataPtr = (double*) malloc(2 * sizeof(double));
            if (dataPtr == NULL)
                STHROW("Unable to allocate memory while reading DataSet: " << objname);
            int posCount = ((struct twodblstruct*)memBuffer)[i].posCount;
            dataObj->posCounts.push_back(posCount);
            dataPtr[0] = ((struct twodblstruct*)memBuffer)[i].data1;
            dataPtr[1] = ((struct twodblstruct*)memBuffer)[i].data2;
            dataObj->posPtrHash.insert(pair<int, void*>(posCount, (void*) dataPtr));
        }
    }
    else {
        STHROW("Unable to read data: unknown DataSet type");
    }
    return dataObj;
}

IEveDataInfo* IEveH5File::getDeviceInfo(string xmlid){
    map<string, IEveDataInfo*>::iterator it = xmlidMap.find("/device/" + xmlid);
    if (it!=xmlidMap.end())
        return new IEveDataInfo(*(it->second));
    else
        return NULL;
}

IEveDataInfo* IEveH5File::getDeviceInfoByName(string name){

    vector<string> devIds = getDeviceIdForName(name);
    if (devIds.size() > 0) {
        // possibly ambigous
        return getDeviceInfo(devIds.front());
    }
    return NULL;
}
