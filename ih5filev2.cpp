#include <iostream>
#include <set>
#include "ih5filev2.h"

using namespace std;

namespace eve {

IH5FileV2::IH5FileV2(H5::H5File oh5file, string filename, float version) : IH5File(oh5file, filename, version)
{
    sections = {"default", "alternate", "meta"};
    calculations = {"averagemeta"};

}

string IH5FileV2::getSectionString(Section sect){
    string chainname = "/c" + to_string(selectedChain);
    switch (sect) {
    case Standard:
        return chainname +"/default";
        break;
    case Snapshot:
        return chainname + "/alternate";
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
