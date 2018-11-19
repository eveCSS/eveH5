#ifndef ICHAINMETADATA_H
#define ICHAINMETADATA_H

#include "attributemetadata.h"
#include "eve.h"

namespace eve {


class IChainMetaData : public AttributeMetaData, public ChainMetaData
{
public:
    IChainMetaData(map<string, string> attribs);
    virtual string getPreferredAxis(){return getString("preferredAxis");};
    virtual string getPreferredChannel(){return getString("preferredChannel");};
    virtual string getPreferredNormalizationChannel(){return getString("PreferredNormalizationChannel");};
    virtual string getStartTime(){return getStartTimeIso();};
    virtual string getEndTime(){return getEndTimeIso();};
};

} // namespace end

#endif // ICHAINMETADATA_H
