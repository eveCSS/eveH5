#ifndef IH5FILEV5_H
#define IH5FILEV5_H

#include "H5Cpp.h"
#include "ih5filev4.h"

namespace eve {

class IH5FileV5 : public IH5FileV4
{
public:
    IH5FileV5(H5::H5File, string, float version);

protected:
    virtual void addExtensionData(IData* data);
};

} // namespace end

#endif // IH5FILEV5_H
