#include "ih5filev5.h"

namespace eve {

IH5FileV5::IH5FileV5(H5::H5File oh5file, string filename) : IH5FileV4(oh5file, filename)
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
    }
    fullh5name = data->getPath() + "averagemeta/" + data->getH5name() + "__Limit-MaxDev";
    extensionmd = findMetaData(extensionmeta, fullh5name);
    if (extensionmd != NULL){
        IData* avdata = new IData((IMetaData&)*extensionmd);
        readDataPCTwoCol(avdata);
        copyAndFill(avdata, DTfloat64, DBLVECT1, data, DTfloat64, AVLIMIT);
        copyAndFill(avdata, DTfloat64, DBLVECT2, data, DTfloat64, AVMAXDEV);
    }
    fullh5name = data->getPath() + "averagemeta/" + data->getH5name() + "__Attempts";
    extensionmd = findMetaData(extensionmeta, fullh5name);
    if (extensionmd != NULL){
        IData* avdata = new IData((IMetaData&)*extensionmd);
        readDataPCTwoCol(avdata);
        copyAndFill(avdata, DTint32, INTVECT1, data, DTint32, AVATT);
        copyAndFill(avdata, DTint32, INTVECT2, data, DTint32, AVATTPR);
    }
    fullh5name = data->getPath() + "standarddev/" + data->getH5name() + "__Count";
    extensionmd = findMetaData(extensionmeta, fullh5name);
    if (extensionmd != NULL){
        IData* avdata = new IData((IMetaData&)*extensionmd);
        readDataPCOneCol(avdata);
        copyAndFill(avdata, DTint32, INTVECT1, data, DTint32, STDDEVCOUNT);
    }
    fullh5name = data->getPath() + "standarddev/" + data->getH5name() + "__TrigIntv-StdDev";
    extensionmd = findMetaData(extensionmeta, fullh5name);
    if (extensionmd != NULL){
        IData* avdata = new IData((IMetaData&)*extensionmd);
        readDataPCTwoCol(avdata);
        copyAndFill(avdata, DTfloat64, DBLVECT1, data, DTfloat64, TRIGGERINTV);
        copyAndFill(avdata, DTfloat64, DBLVECT2, data, DTfloat64, STDDEV);
    }
}

} // namespace end
