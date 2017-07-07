#ifndef IH5FILEV2_H
#define IH5FILEV2_H

#include "H5Cpp.h"
#include "IH5File.h"

namespace eve {

class IH5FileV2 : public IH5File
{
public:
    IH5FileV2(H5::H5File, string);
    virtual string getSectionString(Section);


};

} // namespace end

#endif // IH5FILEV2_H
