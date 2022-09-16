#ifndef IFILEMETADATA_H
#define IFILEMETADATA_H

#include "attributemetadata.h"
#include "eve.h"

namespace eve {


class IFileMetaData : public AttributeMetaData, public FileMetaData
{
public:
    IFileMetaData(map<string, string> attribs);
    virtual string getComment() {return getString("Comment");};
    virtual string getH5Version(){return getString("EVEH5Version");};
    virtual string getXmlVersion(){return getString("XMLversion");};
    virtual string getEveVersion(){return getString("Version");};
    virtual string getLocation(){return getString("Location");};
    virtual string getStartTime(){return getStartTimeIso();};
    virtual string getEndTime(){return getEndTimeIso();};
    virtual string getScmlAuthor(){return getString("SCML-Author");};
    virtual string getScmlName(){return getString("SCML-Name");};
    virtual bool isSimulatedData(){return (getString("Simulation").compare("yes") == 0);};
};

} // namespace end

#endif // IFILEMETADATA_H
