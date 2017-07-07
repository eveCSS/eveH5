#include <iostream>
#include "ih5filev4.h"

namespace eve {

IH5FileV4::IH5FileV4(H5::H5File oh5file, string filename) : IH5FileV3(oh5file, filename)
{
    sections = {"main", "snapshot", "meta"};

}

} // namespace end
