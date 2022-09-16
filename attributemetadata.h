#ifndef ATTRIBUTEMETADATA_H
#define ATTRIBUTEMETADATA_H

#include <string>
#include <map>

using namespace std;

namespace eve {


class AttributeMetaData
{
public:
    AttributeMetaData(map<string, string> attribs);
    string makeIsoTimeString(string date, string time);
    string getStartTimeIso();
    string getEndTimeIso();
    string getString(string name);

private:
    map<string, string> attributes;
};

} // namespace end

#endif // ATTRIBUTEMETADATA_H
