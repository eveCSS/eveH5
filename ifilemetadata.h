#ifndef IFILEMETADATA_H
#define IFILEMETADATA_H

#include "attributemetadata.h"
#include "eve.h"

namespace eve {


class IFileMetaData : public AttributeMetaData, public FileMetaData
{
public:
    IFileMetaData(map<string, string> attribs);
    virtual std::string getComment() {return getString("Comment");};
    virtual std::string getH5Version(){return getString("EVEH5Version");};
    virtual std::string getXmlVersion(){return getString("XMLversion");};
    virtual std::string getEveVersion(){return getString("Version");};
    virtual std::string getLocation(){return getString("Location");};
    virtual std::string getStartTime(){return getStartTimeIso();};
    virtual std::string getEndTime(){return getEndTimeIso();};
    virtual std::string getScmlAuthor(){return getString("SCML-Author");};
    virtual std::string getScmlName(){return getString("SCML-Name");};

};

} // namespace end

#endif // IFILEMETADATA_H
