#include <iostream>
#include "ih5filev4.h"

namespace eve {

IH5FileV4::IH5FileV4(H5::H5File oh5file, string filename) : IH5FileV3(oh5file, filename)
{
    // TODO remove
    cout << "H5 file version 4: " << h5version << endl;
    sections = {"main", "snapshot", "meta"};

}

// TODO
// Version 4 braucht noch eine Korrektur für Extension Data für normalisierte Daten
// siehe /messung/test/daten/2017/kw26interval-and-average00001.h5


} // namespace end
