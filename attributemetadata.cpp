#include "attributemetadata.h"

namespace eve {

AttributeMetaData::AttributeMetaData(map<string, string> attribs)
{
    attributes = attribs;
}

string AttributeMetaData::getStartTimeIso(){

    map<string, string>::iterator it = attributes.find("StartTimeISO");
    if (it != attributes.end()) return it->second;
    string datetime;
    it = attributes.find("StartDate");
    if (it != attributes.end()) {
        string tmpstr = it->second;
        datetime = tmpstr.substr(6,4);
        datetime.append("-");
        datetime.append(tmpstr.substr(3,2));
        datetime.append("-");
        datetime.append(tmpstr.substr(0,2));
        datetime.append("T");
        // TODO
        // StartDate = 25.04.2018 ==> convert to 2018-04-25T
    }
    it = attributes.find("StartTime");
    if (it != attributes.end()) datetime.append(it->second);
    return datetime;
}
string AttributeMetaData::getEndTimeIso(){

    map<string, string>::iterator it = attributes.find("EndTimeISO");
    if (it != attributes.end()) return it->second;
    return string();
}

string AttributeMetaData::getString(string name){

    map<string, string>::iterator it = attributes.find(name);
    if (it != attributes.end()) return it->second;

    return string();
}

} // namespace end
