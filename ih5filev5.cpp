#include "ih5filev5.h"

namespace eve {

IH5FileV5::IH5FileV5(H5::H5File oh5file, string filename, float version) : IH5FileV4(oh5file, filename, version)
{
    sections = {"main", "snapshot", "meta"};
}

void IH5FileV5::addExtensionData(IData* data){

    string fullh5name = data->getPath() + "averagemeta/" + data->getH5name() + "__AverageCount";
    MetaData *extensionmd = findMetaData(extensionmeta, fullh5name);
    if (extensionmd != NULL){
        IData* avdata = new IData((IMetaData&)*extensionmd);
        readDataPCTwoCol(avdata);
        copyAndFill(avdata, DTint32, INTVECT1, data, DTint32, AVCOUNT);
        copyAndFill(avdata, DTint32, INTVECT2, data, DTint32, AVCOUNTPR);
        delete avdata;
    }
    fullh5name = data->getPath() + "averagemeta/" + data->getH5name() + "__Limit-MaxDev";
    extensionmd = findMetaData(extensionmeta, fullh5name);
    if (extensionmd != NULL){
        IData* avdata = new IData((IMetaData&)*extensionmd);
        readDataPCTwoCol(avdata);
        copyAndFill(avdata, DTfloat64, DBLVECT1, data, DTfloat64, AVLIMIT);
        copyAndFill(avdata, DTfloat64, DBLVECT2, data, DTfloat64, AVMAXDEV);
        delete avdata;
    }
    fullh5name = data->getPath() + "averagemeta/" + data->getH5name() + "__Attempts";
    extensionmd = findMetaData(extensionmeta, fullh5name);
    if (extensionmd != NULL){
        IData* avdata = new IData((IMetaData&)*extensionmd);
        readDataPCTwoCol(avdata);
        copyAndFill(avdata, DTint32, INTVECT1, data, DTint32, AVATT);
        copyAndFill(avdata, DTint32, INTVECT2, data, DTint32, AVATTPR);
        delete avdata;
    }
    fullh5name = data->getPath() + "standarddev/" + data->getH5name() + "__Count";
    extensionmd = findMetaData(extensionmeta, fullh5name);
    if (extensionmd != NULL){
        IData* avdata = new IData((IMetaData&)*extensionmd);
        readDataPCOneCol(avdata);
        copyAndFill(avdata, DTint32, INTVECT1, data, DTint32, STDDEVCOUNT);
        delete avdata;
    }
    fullh5name = data->getPath() + "standarddev/" + data->getH5name() + "__TrigIntv-StdDev";
    extensionmd = findMetaData(extensionmeta, fullh5name);
    if (extensionmd != NULL){
        IData* avdata = new IData((IMetaData&)*extensionmd);
        readDataPCTwoCol(avdata);
        copyAndFill(avdata, DTfloat64, DBLVECT1, data, DTfloat64, TRIGGERINTV);
        copyAndFill(avdata, DTfloat64, DBLVECT2, data, DTfloat64, STDDEV);
        delete avdata;
    }
}

vector<string> IH5FileV5::getLogData(){

    vector<string> stringlist;
    StrType tid1(0, H5T_VARIABLE);
    hid_t		native_type;
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
    if((native_type=H5Tget_native_type(h5dtype.getId(), H5T_DIR_DEFAULT)) < 0 )
        cerr << "get LiveComment: H5Tget_native_type  failed!!! " << endl;

    /* Check if the data type is equal */
    if(!H5Tequal(native_type, tid1.getId()))
        cerr << "get LiveComment: native type is not var length string!!!" << endl;

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
        if (rdata[i] != NULL){
            stringlist.push_back(string(rdata[i]));
            free(rdata[i]);
        }
    }
    return stringlist;
}

} // namespace end
