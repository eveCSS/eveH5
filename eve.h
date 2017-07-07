#ifndef EVEH5_H
#define EVEH5_H

#include <string>
#include <map>
#include <vector>
#include <list>

namespace eve {

/** fill rule specifies how to create joined data
*
*/
enum FillRule
{
    NoFill,     /**< use only columns with the same position reference */
    LastFill,   /**< fill in the last position if an axis has no value at the specified position reference (assume the axis hasn't moved) */
    NANFill,    /**< fill in NaN, if a channel has no value at the specified position reference*/
    LastNANFill /**< do LastFill and NANFill */
};

/** device type (channel or axis)
*
*/
enum DeviceType
{
    Unknown,    /**< unknown */
    Channel,    /**< channel  */
    Axis        /**< axis  */
};

/** section (data to work with)
*
*/
enum Section
{
    Standard,    /**< standard data */
    Snapshot,    /**< snapshot data */
    Monitor,     /**< monitor data  */
    Timestamp,   /**< Timestamps    */
};

/** data type
*
*/
enum DataType
{
    DTunknown,     /**<  unknown type */
    DTstring,      /**<  (C++) string  */
    DTint32,       /**<  32bit integer */
    DTfloat64,     /**<  64bit double */
    DTint8,
    DTint16,
    DTint64,
    DTuint8,
    DTuint16,
    DTuint32,
    DTuint64,
    DTfloat32
};

class MetaData
{
public:
    virtual ~MetaData(){};

    /** get the device name
     * \return name
     */
    virtual std::string getName()=0;

    /** get the unit
     * \return unit
     */
    virtual std::string getUnit()=0;

    /** get the id as used in the xml-file (XML-ID)
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

    /** get section
     * \return name
     */
    virtual Section getSection()=0;

    /** get a hash with attributes as key/value pairs
     * \return return a key/value hash
     */
    virtual std::map<std::string, std::string>& getAttributes()=0;

    /** get the device type (axis, channel)
     * \return device type
     */
    virtual DeviceType getDeviceType()=0;

    /** get data type
     * \return datatype
     */
    virtual DataType getDataType()=0;

};

class Data : public MetaData
{
public:
    virtual ~Data(){};

    /** get position references
     * \return array of position references
     */
    virtual std::vector<int> getPosReferences()=0;

    /** get pointer to internal array data (for array data only).
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

    /** check if data is array data.
     * \return true if array data
     */
    virtual bool isArrayData()=0;

    /** check if data has average data.
     * \return true if data has average data
     */
    virtual bool hasAverageData()=0;

    /** check if data has standard deviation data.
     * \return true if data has standard deviation data
     */
    virtual bool hasStdDeviation()=0;

    /** get maximum allowed attempts for limit measurements
     * \return list of maximum allowed attempts
     */
    virtual std::vector<int> getAverageAttemptsPreset()=0;

    /** get used attempts for limit measurements
     * \return list of used attempts
     */
    virtual std::vector<int> getAverageAttempts()=0;

    /** get preset count of measurements for average measurements
     * \return list of counts
     */
    virtual std::vector<int> getAverageCountPreset()=0;

    /** get used measurement count for limit measurements
     * \return list of used counts
     */
    virtual std::vector<int> getAverageCount()=0;

    /** get preset limit
     * \return list of preset limits
     */
    virtual std::vector<double> getAverageLimitPreset()=0;

    /** get preset allowed maximum deviation
     * \return list of allowed maximum deviation
     */
    virtual std::vector<double> getAverageMaxDeviationPreset()=0;


    /** get count of measurements for interval detectors
     * \return list of counts
     */
    virtual std::vector<int> getStddevCount()=0;

    /** get standard deviation for interval detectors
     * \return list of standard deviation
     */
    virtual std::vector<double> getStddeviation()=0;

    /** get length of trigger interval in s
     * \return list of trigger interval
     */
    virtual std::vector<double> getTriggerIntv()=0;

};

class JoinedData
{
public:
    virtual ~JoinedData(){};

    /** join a list of data objects to retrieve a list of data objects with corresponding
     * values. May be used to create table data from single data objects. All data will have the
     * same number of values.
     *
     * \param datalist list with Data objects
     * \param fill desired fill rule
     * \return JoinedData
     */
    static JoinedData* getJoinedData(std::vector<Data*>* datalist, FillRule fill=NoFill);

    /** retrieve metadata of a specific column
     *
     * \param col column number (start with 0)
     * \return metadata
     */
    virtual MetaData* getMetaData(unsigned int col)=0;

    /** get the data object of the specified column.
     *
     * Use this to retrieve a data object with values as specified in fill rule
     * Use getColumnType() to retrieve the type of the data
     * \param col column number (start with 0)
     * \return pointer to data or NULL if an error occurs
     * \sa getColumnType()
     */
    virtual Data* getData(unsigned int col)=0;

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

};


class DataFile {
public:
    virtual ~DataFile(){};

    /** Open a Data File (usually H5 format).
     * \param name Name of file to open
     * \return DataFile object
     */
    static DataFile* openFile(std::string name);

    /** get a list of all available chains.
     * \return list of chains
     */
    virtual std::vector<int> getChains()=0;

    /** Retrieve selected chain.
     * \return id of selected chain
     */
    virtual int getChain()=0;

    /** Set chain as selected chain (chain 1 selected as default).
     * \param chain id of an available chain
     */
    virtual void setChain(int chain)=0;

    /** Retrieve a hash with metadata of selected chain as key/value pairs.
     * \return return a key/value hash
     */
    virtual std::map<std::string, std::string> getChainMetaData()=0;

    /** Retrieve a hash with file metadata  as key/value pairs.
     * \return return a key/value hash
     */
    virtual std::map<std::string, std::string> getFileMetaData()=0;

    /**
     * @brief Retrieve metadata objects for the specified section in selected chain.
     * @param section
     * @param filter select only metadata which contain this string as XML-ID
     * @return MetaData list of metadata pointers
     */
    virtual std::vector<MetaData *> getMetaData(Section section, std::string filter="")=0;

    /** Retrieve a list of data objects.
     *
     * \param metadatalist list of metadata to retrieve data for
     * \return list of data pointers (delete after use)
     */
    virtual std::vector<Data*> getData(std::vector<MetaData*>& metadatalist)=0;

    /** Retrieve joined data for given metada.
     *
     * \param metadatalist list of metadata to retrieve data for
     * \param fill desired fill rule
     * \return list of data pointers (delete after use)
     */
    virtual JoinedData* getJoinedData(std::vector<MetaData*>& metadatalist, FillRule fill=NoFill)=0;

    /** Retrieve joined data for data marked as preferred in selected chain.
     *
     * \param fill desired fill rule
     * \return list of data pointers (delete after use)
     */
    virtual JoinedData* getPreferredData(FillRule fill=NoFill)=0;

    /** Retrieve log
     *
     * \return list of log messages
     */
    virtual std::vector<std::string> getLogData()=0;

};

} // namespace end


/** \example main.cpp
 * This is an example of how to use the EveH5 classes.
 *
 * Link this to the shared library libeveH5.so
 */


#endif // EVEH5_H
