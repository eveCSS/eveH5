#include <iostream>
#include "ih5filev4.h"

namespace eve {

IH5FileV4::IH5FileV4(H5::H5File oh5file, string filename, float version) : IH5FileV3(oh5file, filename, version)
{
    sections = {"main", "snapshot", "meta"};

}

string IH5FileV4::getSectionString(Section sect){
    string chainname = "/c" + to_string(selectedChain);
    switch (sect) {
    case Standard:
        return chainname +"/main";
        break;
    case Snapshot:
        return chainname + "/snapshot";
        break;
    case Monitor:
        return "/device";
        break;
    case Timestamp:
        return chainname + "/meta";
        break;
    default:
        break;
    }
    return "unknown";
}

} // namespace end
