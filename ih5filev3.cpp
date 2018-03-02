#include <iostream>
#include "ih5filev3.h"

namespace eve {

IH5FileV3::IH5FileV3(H5::H5File oh5file, string filename, float version) : IH5FileV2(oh5file, filename, version)
{
    sections = {"default", "alternate", "meta"};
    calculations = {"standarddev", "averagemeta"};
}

} // namespace end
