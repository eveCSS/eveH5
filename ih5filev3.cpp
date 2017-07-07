#include <iostream>
#include "ih5filev3.h"

namespace eve {

IH5FileV3::IH5FileV3(H5::H5File oh5file, string filename) : IH5FileV2(oh5file, filename)
{
    sections = {"main", "snapshot", "meta"};
    calculations = {"standarddev", "averagemeta"};
}

string IH5FileV3::getSectionString(Section sect){
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
