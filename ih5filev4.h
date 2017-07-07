#ifndef IH5FILEV4_H
#define IH5FILEV4_H

#include "H5Cpp.h"
#include "ih5filev3.h"

namespace eve {

class IH5FileV4 : public IH5FileV3
{
public:
    IH5FileV4(H5::H5File, string);
};

} // namespace end

#endif // IH5FILEV4_H
