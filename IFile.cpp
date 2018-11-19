#include <string.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <set>
#include "IFile.h"
#include "IH5File.h"
#include <H5Exception.h>
#include "ih5filev5.h"

using namespace std;

#define STHROW(msg) { \
     ostringstream err;\
     err<<msg; \
     throw runtime_error(err.str()); }


#define EVEH5VERSIONMAXIMUM 5.99
#define EVEH5VERSIONMINIMUM 1.0


namespace eve {

DataFile* DataFile::openFile(string name){return new IFile(name);};

IFile::IFile(string filename)
{
    ih5file = NULL;
    H5File h5file;
    h5version = 0.0;

    Exception::dontPrint();

    try {
        if (H5File::isHdf5(filename))
            h5file.openFile(filename, H5F_ACC_RDONLY);
        else {
            STHROW("Error opening file " << filename << "; unsupported file format " );
            return;
        }
    }
    catch (H5::Exception error){
        STHROW("Error opening file " << filename << "; H5 Error: " << error.getDetailMsg() );
    }

    Group root;
    openGroupH5(h5file, root, "/");
    getH5Version(root);
    closeGroupH5(root);

    if ((h5version > EVEH5VERSIONMAXIMUM) || (h5version < EVEH5VERSIONMINIMUM))
        STHROW("EVEH5 version mismatch, file " << h5version << ", supported "
               << (float)EVEH5VERSIONMINIMUM << " - " << (float)EVEH5VERSIONMAXIMUM << "\n");

    if (h5version >= 5.0)
        ih5file = new IH5FileV5(h5file, filename, h5version);
    else if (h5version >= 4.0)
        ih5file = new IH5FileV4(h5file, filename, h5version);
    else if (h5version >= 3.0)
        ih5file = new IH5FileV3(h5file, filename, h5version);
    else if (h5version >= 2.0)
        ih5file = new IH5FileV2(h5file, filename, h5version);
    else
        ih5file = new IH5File(h5file, filename, h5version);

    try {
        ih5file->init();
    }
    catch (Exception error){
        string errormessage = "";

        try {
            if (ih5file != NULL) delete ih5file;
        }
        catch(Exception error){
            errormessage = error.getDetailMsg();
        }

        ih5file = NULL;
        STHROW("Error during init" << filename << "; H5 Error: " << error.getDetailMsg() );
    }
    return;
}

IFile::~IFile()
{
    if (ih5file != NULL) delete ih5file;
}

void IFile::openGroupH5(H5File h5file, Group& h5group, string path){
    try {
        h5group = h5file.openGroup(path);
    }
    catch (Exception error){
        STHROW("Error opening group " << path << "; H5 Error: " << error.getDetailMsg() );
    }
}

void IFile::closeGroupH5(Group& h5group){
    try {
        h5group.close();
    }
    catch (Exception error){
        STHROW("Error closing group; H5 Error: " << error.getDetailMsg() );
    }
}

void IFile::getH5Version(Group& rootgroup){

    Attribute versionattr;
    try {
        versionattr = rootgroup.openAttribute("EVEH5Version");
        StrType stread = StrType(versionattr.getStrType());
        string versionString;
        versionattr.read(stread, versionString);
        h5version = strtof(versionString.c_str(), NULL);
    }
    catch (Exception error){
        cout << "No EVEH5 version information available, assuming EVEH5 version 1.0" << endl;
        h5version = 1.0;
    }
    if ((h5version > EVEH5VERSIONMAXIMUM) || (h5version < EVEH5VERSIONMINIMUM))
        STHROW("EVEH5 version mismatch, file " << h5version << ", supported "
               << (float)EVEH5VERSIONMINIMUM << " - " << (float)EVEH5VERSIONMAXIMUM << "\n");
    return;
}

//void IFile::close(H5File h5file)
//{
//    try {
//        h5file.close();
//    }
//    catch (Exception error){
//        STHROW("Error closing file; H5 Error: " << error.getDetailMsg() );
//    }
//}

} // namespace end
