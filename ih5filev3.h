#ifndef IH5FILEV3_H
#define IH5FILEV3_H

#include "ih5filev2.h"

namespace eve {

class IH5FileV3 : public IH5FileV2
{
public:
    IH5FileV3(H5::H5File, string, float version);
};

} // namespace end

#endif // IH5FILEV4_3
