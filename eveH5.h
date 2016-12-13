#ifndef EVEH5_H
#define EVEH5_H

#include <string>
#include <map>
#include <vector>
#include <list>

/** fill type specifies how to create joined data
*
*/
enum EVEFillType
{
    NoFill,     /**< use only columns with the same position reference */
    LastFill,   /**< fill in the last position if an axis has no value at the specified position reference (assume the axis hasn't moved) */
    NANFill,    /**< fill in NaN, if a channel has no value at the specified position reference*/
    LastNANFill /**< do LastFill and NANFill */
};
/** device type (channel or axis)
*
*/
enum EVEDeviceType
{
    DEVTUnknown,    /** unknown */
    DEVTChannel,    /** channel  */
    DEVTAxis        /** axis  */
};
/** data type
*
*/
enum EVEDataType
{
    DTunknown,     /**  unknown (not existing) type */
    DTstring,      /**  (C++) string  */
    DTint32,       /**  32bit integer */
    DTfloat64,     /**  64bit double */
    DTint8,
    DTint16,
    DTint64,
    DTuint8,
    DTuint16,
    DTuint32,
    DTuint64,
    DTfloat32
};

class EveDataInfo
{
public:
    virtual ~EveDataInfo(){};

    /** get the id as used in the xml-file
     * \return id-string
     */
    virtual std::string getId()=0;

    /** get the channel identification string
     * \return channel-id
     */
    virtual std::string getChannelId()=0;

    /** get the id of channel used for normalization
     * \return normalization-id
     */
    virtual std::string getNormalizeId()=0;

    /** get the number of rows and columns
      for array data columns is > 1
     * \return rows, columns
     */
    virtual std::pair<int, int> getDimension()=0;

    /** get a hash with attributes as key/value pairs
     * \return return a key/value hash
     */
    virtual std::multimap<std::string, std::string>& getAttributes()=0;

    /** get the device type (axis, channel)
     * \return device type
     */
    virtual EVEDeviceType getDeviceType()=0;

    /** get data type
     * \return datatype
     */
    virtual EVEDataType getDataType()=0;

};

class EveData : public EveDataInfo
{
public:
    virtual ~EveData(){};

    /** get position references
     * \return array of position references
     */
    virtual std::vector<int> getPosReferences()=0;

    /** Get pointer to internal array data (for array data only).
     * Use this to retrieve array data for a single position reference
     * \param posRef position reference for the desired array data
     * \param ptr address where the data pointer will be stored
     * \return number of values in the array or -1 if not array data
     * \sa isArrayData()
     */
    virtual int getArrayDataPointer(int posRef, void** ptr)=0;

    /** get pointer to internal data container (not for array data).
     * Use this to retrieve a container with all values i.e for all position references
     * \param ptr address where the data pointer will be stored
     * \return number of values in the array or -1 if array data
     * \sa isArrayData()
     */
    virtual int getDataPointer(void** ptr)=0;
    /** Check if data is array data.
     * \return true if array data
     */
    virtual bool isArrayData()=0;
};

class EveH5File {
public:
    virtual ~EveH5File(){};

    /** Open a H5 File in H5 Format.
     * \param name Name of H5 file to open
     * \return EveH5File object
     */
    static EveH5File* openH5File(std::string name);

    /** Retrieve chain names.
     * \return list of chain names
     */
    virtual std::vector<std::string> getChainNames()=0;

    /** Retrieve the user-defined device name.
     *
     *Devices may have a user-defined name.
     * \return list of device names
     */
    virtual std::vector<std::string> getDeviceNames()=0;

    /** Retrieve the device ids.
     *
     *All devices have a unique id.
     * \return list of device ids
     */
    virtual std::vector<std::string> getDeviceIds()=0;

    /** Retrieve the device ids with monitor data.
     *
     * \return list of monitored device ids
     */
    virtual std::vector<std::string> getMonitorDeviceIds()=0;

    /** Retrieve the device id for a given device name.
     *
     * A name may be defined for more than one device (not recommended)
     * \param name device name
     * \return list of device ids
     */
    virtual std::vector<std::string> getDeviceIdForName(std::string name)=0;

    /**
     * @brief getPathsForId get all full paths to data for given id
     * @param id
     * @return list of fully qualified dataset names
     */
    virtual std::vector<std::string> getPathsForId(std::string id)=0;

    /**
     * @brief getDataInfoForPath retrieve an info object for a specified data path
     * @param path
     * @return EveDataInfo pointer (delete EveDataInfo if not used any more)
     */
    virtual EveDataInfo* getDataInfoForPath(std::string path)=0;

    /**
     * @brief getMonitorDataInfo retrieve an info object for a monitored device
     * @param id id of a monitored device
     * @return  EveDataInfo pointer (delete after use)
     */
    virtual EveDataInfo* getMonitorDataInfo(std::string id)=0;

    /** Retrieve data described by dataInfo
     *
     * \param dataInfo EveDataInfo pointer
     * \return EveData object pointer (delete after use)
     */
    virtual EveData* getData(EveDataInfo* dataInfo)=0;

    /** retrieve a hash with root (file) attributes as key/value pairs
     * \return return a key/value hash
     */
    virtual std::multimap<std::string, std::string> getRootAttributes()=0;

    /** retrieve a hash with chain (group) attributes of given chain as key/value pairs
     * \return return a key/value hash
     */
    virtual std::multimap<std::string, std::string> getChainAttributes(std::string chain)=0;
};

class EveJoinData
{
public:
    virtual ~EveJoinData(){};
    /** join a list of single-column data objects to retrieve multi-column data
     *
     * \param datalist list with EveData objects
     * \param filltype desired filltype used for calculation
     * \return EveJoinData object pointer
     */
    static EveJoinData* getCombinedData(std::vector<EveData*>* datalist, EVEFillType filltype);
    /** Retrieve the device id of a specific column
     *
     * \param col column number (start with 0)
     * \return device id
     */
    virtual std::string getColumnId(unsigned int col)=0;
    /** get pointer to internal data container of the specified column.
     *
     * Use this to retrieve a container with all values i.e for all position references
     * Use getColumnType() to retrieve the type of the container
     * \param col column number (start with 0)
     * \param ptr address where the data pointer will be stored
     * \return number of values in the array or -1 if array data
     * \sa getColumnType()
     */
    virtual int getColumnPointer(unsigned int col, void** ptr)=0;
    /** Retrieve the number of columns
     *
     * \return number of columns
     */
    virtual unsigned int getColumnCount()=0;
    /** Retrieve the number of rows (size of each column array)
     *
     * \return number of rows
     */
    virtual unsigned int getValueCount()=0;
    /** Retrieve the datatype of a specific column
     *
     * \param col column number (start with 0)
     * \return datatype
     */
    virtual EVEDataType getColumnType(unsigned int col)=0;
    /** retrieve position references
     * \return array of position references
     */
    virtual std::vector<int> getPosReferences()=0;

};

/** \example main.cpp
 * This is an example of how to use the EveH5 classes.
 *
 * Link this to the shared library libeveH5.so
 */

#endif // EVEH5_H
