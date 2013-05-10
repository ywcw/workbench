
/*LICENSE_START*/
/*
 * Copyright 2013 Washington University,
 * All rights reserved.
 *
 * Connectome DB and Connectome Workbench are part of the integrated Connectome 
 * Informatics Platform.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of Washington University nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR  
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*LICENSE_END*/

#include <set>

#define __CIFTI_MAPPABLE_DATA_FILE_DECLARE__
#include "CiftiMappableDataFile.h"
#undef __CIFTI_MAPPABLE_DATA_FILE_DECLARE__

#include "BoundingBox.h"
#include "CaretAssert.h"
#include "CaretLogger.h"
#include "CiftiFacade.h"
#include "CiftiFile.h"
#include "CiftiInterface.h"
#include "CaretTemporaryFile.h"
#include "CiftiXnat.h"
#include "CiftiXML.h"
#include "DescriptiveStatistics.h"
#include "FastStatistics.h"
#include "FileInformation.h"
#include "GiftiLabel.h"
#include "GiftiLabelTable.h"
#include "GiftiMetaData.h"
#include "GroupAndNameHierarchyModel.h"
#include "Histogram.h"
#include "NodeAndVoxelColoring.h"
#include "PaletteColorMapping.h"
#include "PaletteFile.h"
#include "SparseVolumeIndexer.h"

using namespace caret;


    
/**
 * \class caret::CiftiMappableDataFile 
 * \brief Abstract class for CIFTI files that are mapped to surfaces and volumes
 * \ingroup Files
 */

/**
 * Constructor.
 *
 * @param dataFileType
 *    Type of data file.
 * @param fileReading
 *    How to read data from the file 
 * @param rowIndexType
 *    Type of CIFTI indexing along the rows (dimension 0) for the file.
 * @param columnIndexType
 *    Type of CIFTI indexing along the columns (dimension 1) for the file.
 * @param brainordinateMappedDataAccess
 *    Location of the brainordinate data
 * @param seriesDataAccess
 *    Location of the series data
 */
CiftiMappableDataFile::CiftiMappableDataFile(const DataFileTypeEnum::Enum dataFileType,
                                             const FileReading fileReading,
                                             const IndicesMapToDataType rowIndexType,
                                             const IndicesMapToDataType columnIndexType,
                                             const DataAccess brainordinateMappedDataAccess,
                                             const DataAccess seriesDataAccess)
: CaretMappableDataFile(dataFileType),
m_fileReading(fileReading),
m_requiredRowIndexType(rowIndexType),
m_requiredColumnIndexType(columnIndexType),
m_brainordinateMappedDataAccess(brainordinateMappedDataAccess),
m_seriesDataAccess(seriesDataAccess)
{
    m_classNameHierarchy.grabNew(new GroupAndNameHierarchyModel());
    m_forceUpdateOfGroupAndNameHierarchy = true;
    
    m_ciftiFacade.grabNew(NULL);
    m_ciftiInterface.grabNew(NULL);
    m_metadata.grabNew(new GiftiMetaData());
    m_voxelIndicesToOffset.grabNew(NULL);
    clearPrivate();
}

/**
 * Destructor.
 */
CiftiMappableDataFile::~CiftiMappableDataFile()
{
    clearPrivate();
}

/**
 * Clear the contents of the file.
 */
void
CiftiMappableDataFile::clear()
{
    CaretMappableDataFile::clear();
    clearPrivate();
}

/**
 * Clear the contents of the file.
 * Note that "clear()" is virtual and cannot be called from destructor.
 */
void
CiftiMappableDataFile::clearPrivate()
{
    m_ciftiFacade.grabNew(NULL);
    m_ciftiInterface.grabNew(NULL);
    m_metadata->clear();
    m_voxelIndicesToOffset.grabNew(NULL);

    const int64_t num = static_cast<int64_t>(m_mapContent.size());
    for (int64_t i = 0; i < num; i++) {
        delete m_mapContent[i];
    }
    m_mapContent.clear();
    m_containsVolumeData  = false;
    
    m_volumeDimensions[0] = 0;
    m_volumeDimensions[1] = 0;
    m_volumeDimensions[2] = 0;
    m_volumeDimensions[3] = 0;
    m_volumeDimensions[4] = 0;
    
    m_classNameHierarchy->clear();
    m_forceUpdateOfGroupAndNameHierarchy = true;
    
}

/**
 * @return Is this file empty?
 */
bool
CiftiMappableDataFile::isEmpty() const
{
    if (getNumberOfMaps() > 0) {
        return false;
    }
    return true;
}

/**
 * @return structure file maps to.
 */
StructureEnum::Enum
CiftiMappableDataFile::getStructure() const
{
    /*
     * CIFTI files apply to all structures.
     */
    return StructureEnum::ALL;
}

/**
 * Set the structure to which file maps.
 * @param structure
 *    New structure to which file maps.
 */
void
CiftiMappableDataFile::setStructure(const StructureEnum::Enum /*structure*/)
{
    /* CIFTI files may apply to all structures */
}

/**
 * @return Metadata for the file.
 */
GiftiMetaData*
CiftiMappableDataFile::getFileMetaData()
{
    return m_metadata;
}

/**
 * @return Metadata for the file.
 */
const GiftiMetaData*
CiftiMappableDataFile:: getFileMetaData() const
{
    return m_metadata;
}

/**
 * Read the file.
 *
 * @param filename
 *    Name of the file to read.
 * @throw
 *    DataFileException if there is an error reading the file.
 */
void
CiftiMappableDataFile::readFile(const AString& filename) throw (DataFileException)
{
    clear();
    
    try {
        /*
         * Is the file on the network (name begins with http, ftp, etc.).
         */
        if (DataFile::isFileOnNetwork(filename)) {
            /*
             * Data in Xnat does not end with a valid file extension
             * but ends with HTTP search parameters.  Thus, if the
             * filename does not have a valid extension, assume that
             * the data is in Xnat.
             */
            bool isValidFileExtension = false;
            DataFileTypeEnum::fromName(filename,
                                       &isValidFileExtension);
            
            if (isValidFileExtension) {
                switch (m_fileReading) {
                    case FILE_READ_DATA_ALL:
                        break;
                    case FILE_READ_DATA_AS_NEEDED:
                        throw DataFileException(filename
                                                + " of type "
                                                + DataFileTypeEnum::toGuiName(getDataFileType())
                                                + " cannot be read over the network.  The file must be"
                                                " accessed by reading individual rows and/or columns"
                                                " and this cannot be performed over a network.");
                        break;
                }
                
                CaretTemporaryFile tempFile;
                tempFile.readFile(filename);
                
                CiftiFile* ciftiFile = new CiftiFile();
                ciftiFile->openFile(tempFile.getFileName(),
                                    IN_MEMORY);
                m_ciftiInterface.grabNew(ciftiFile);
            }
            else {
                CiftiXnat* ciftiXnat = new CiftiXnat();
                AString username = "";
                AString password = "";
                AString filenameToOpen = "";
                
                /*
                 * Username and password may be embedded in URL, so extract them.
                 */
                FileInformation fileInfo(filename);
                fileInfo.getRemoteUrlUsernameAndPassword(filenameToOpen,
                                                         username,
                                                         password);
                
                /*
                 * Always override with a password entered by the user.
                 */
                if (CaretDataFile::getFileReadingUsername().isEmpty() == false) {
                    username = CaretDataFile::getFileReadingUsername();
                    password = CaretDataFile::getFileReadingPassword();
                }
                
                ciftiXnat->setAuthentication(filenameToOpen,
                                             username,
                                             password);
                ciftiXnat->openURL(filenameToOpen);
                m_ciftiInterface.grabNew(ciftiXnat);
            }
        }
        else {
            CiftiFile* ciftiFile = new CiftiFile();
            switch (m_fileReading) {
                case FILE_READ_DATA_ALL:
                    ciftiFile->openFile(filename,
                                        IN_MEMORY);
                    break;
                case FILE_READ_DATA_AS_NEEDED:
                    ciftiFile->openFile(filename,
                                        ON_DISK);
                    break;
            }
            m_ciftiInterface.grabNew(ciftiFile);
        }
        
        /*
         * Need a pointer to the CIFTI XML.
         */
        CaretAssert(m_ciftiInterface);
        const CiftiXML& ciftiXML = m_ciftiInterface->getCiftiXML();
        
        setFileName(filename);
        
        /*
         * Create the CIFTI facade for simplified access to CIFTI data
         */
        m_ciftiFacade.grabNew(new CiftiFacade(getDataFileType(),
                                              m_ciftiInterface));
        AString errorMessage;
        if (m_ciftiFacade->isValidCiftiFile() == false) {
            errorMessage.appendWithNewLine("Support for "
                                           + DataFileTypeEnum::toName(getDataFileType())
                                           + " needs to be implemented or is invalid type.");
        }
        
        /*
         * Get contents of the matrix.
         */
        const IndicesMapToDataType rowIndexTypeInFile = ciftiXML.getMappingType(CiftiXML::ALONG_ROW);
        AString rowIndexTypeName = ciftiIndexTypeToName(rowIndexTypeInFile);
        
        const IndicesMapToDataType columnIndexTypeInFile = ciftiXML.getMappingType(CiftiXML::ALONG_COLUMN);
        AString columnIndexTypeName = ciftiIndexTypeToName(columnIndexTypeInFile);
        
        /*
         * Validate type of data in rows and columns
         */
        if (m_requiredRowIndexType != rowIndexTypeInFile) {
            errorMessage.appendWithNewLine("Row Index Type should be "
                                           + ciftiIndexTypeToName(m_requiredRowIndexType)
                                           + " but is "
                                           + rowIndexTypeName);
        }
        if (m_requiredColumnIndexType != columnIndexTypeInFile) {
            errorMessage.appendWithNewLine("Column Index Type should be "
                                           + ciftiIndexTypeToName(m_requiredColumnIndexType)
                                           + " but is "
                                           + columnIndexTypeName);
        }
        
        
        if (m_ciftiFacade->isBrainordinateDataColoredWithLabelTable()) {
            /* OK */
        }
        else if (m_ciftiFacade->isBrainordinateDataColoredWithPalette()) {
            /* OK */
        }
        else {
            errorMessage.appendWithNewLine("Data is neither color with label table nor palette.");
        }
        
        if (errorMessage.isEmpty() == false) {
            errorMessage = (getFileNameNoPath()
                            + errorMessage);
            clear();
            throw DataFileException(errorMessage);
        }
        
        /*
         * Copy the file metadata into a GiftiMetaData object.
         */
        m_ciftiFacade->getFileMetadata(m_metadata);
        
        /*
         * Get data for maps.
         */
        const int32_t numberOfMaps = m_ciftiFacade->getNumberOfMaps();
        for (int32_t i = 0; i < numberOfMaps; i++) {            
            MapContent* mc = new MapContent(m_ciftiFacade,
                                            i);
            m_mapContent.push_back(mc);
        }
        
        /*
         * Setup voxel mapping
         */
        std::vector<CiftiVolumeMap> voxelMapping;
        m_ciftiFacade->getVolumeMapForMappingDataToBrainordinates(voxelMapping);
        m_voxelIndicesToOffset.grabNew(new SparseVolumeIndexer(m_ciftiInterface,
                                                               voxelMapping));
        
        /*
         * Indicate if volume mappable
         */
        if (m_voxelIndicesToOffset->isValid()) {
            m_containsVolumeData = true;
            
            VolumeBase::OrientTypes orient[3];
            int64_t dimensions[3];
            float origin[3];
            float spacing[3];
            if (ciftiXML.getVolumeAttributesForPlumb(orient,
                                                 dimensions,
                                                 origin,
                                                     spacing)) {
                m_volumeDimensions[0] = dimensions[0];
                m_volumeDimensions[1] = dimensions[1];
                m_volumeDimensions[2] = dimensions[2];
                m_volumeDimensions[3] = 1;
                m_volumeDimensions[4] = 1;
            }
        }
        
        AString mapNames;
        if (hasMapAttributes()) {
            for (int32_t i = 0; i < getNumberOfMaps(); i++) {
                mapNames.appendWithNewLine("        Map "
                                           + AString::number(i)
                                           + " Name: "
                                           + getMapName(i));
            }
            if (mapNames.isEmpty() == false) {
                mapNames.insert(0, "\n");
            }
        }
        
        /*
         * Map units
         */
        float startValue;
        float stepValue;
        NiftiTimeUnitsEnum::Enum units;
        m_ciftiFacade->getMapIntervalStartStepAndUnits(startValue,
                                                       stepValue,
                                                       units);
        const AString unitString(NiftiTimeUnitsEnum::toName(units)
                                 + ", "
                                 + AString::number(startValue)
                                 + ", "
                                 + AString::number(stepValue));
        
        const AString msg = (getFileNameNoPath()
                             + "\n   " + DataFileTypeEnum::toGuiName(getDataFileType())
                             + "\n   Rows: " + AString::number(m_ciftiFacade->getNumberOfRows())
                             + "\n   Columns: " + AString::number(m_ciftiFacade->getNumberOfColumns())
                             + "\n   RowType: " + rowIndexTypeName
                             + "\n   ColType: " + columnIndexTypeName
                             + "\n   Has Surface Data: " + AString::fromBool(m_ciftiFacade->containsSurfaceDataForMappingToBrainordinates())
                             + "\n   Has Volume Data: " + AString::fromBool(m_containsVolumeData)
                             + "\n   Voxel Count: " + AString::number(voxelMapping.size())
                             + "\n   Volume Dimensions: " + AString::fromNumbers(m_volumeDimensions, 5, ",")
                             + "\n   Number of Maps: " + AString::number(m_mapContent.size())
                             + mapNames
                             + "\n   Map Units, Start, Stop: " + unitString
                             + "\n   Map with Label Table: " + AString::fromBool(m_ciftiFacade->isBrainordinateDataColoredWithLabelTable())
                             + "\n   Map With Palette: " + AString::fromBool(m_ciftiFacade->isBrainordinateDataColoredWithPalette()));
                             
        CaretLogFine(msg);
        
        clearModified();
    }
    catch (CiftiFileException& e) {
        clear();
        throw DataFileException(e.whatString());
    }
    
    m_classNameHierarchy->update(this,
                                 true);
    m_forceUpdateOfGroupAndNameHierarchy = false;
    m_classNameHierarchy->setAllSelected(true);

    CaretLogFiner("CLASS/NAME Table for : "
                  + this->getFileNameNoPath()
                  + "\n"
                  + m_classNameHierarchy->toString());

    validateKeysAndLabels();
    
    validateAfterFileReading();
}

/**
 * This method is intended for overriding by subclasess so that they 
 * can examine and verify the data that was read.  This method is
 * called after successfully reading a file.
 */
void
CiftiMappableDataFile::validateAfterFileReading() throw (DataFileException)
{
    /* nothing - see method comment. */
}

/**
 * Write the file.
 *
 * @param filename
 *    Name of the file to write.
 * @throw
 *    DataFileException if there is an error writing the file.
 */
void
CiftiMappableDataFile::writeFile(const AString& filename) throw (DataFileException)
{
    try {
        if (m_ciftiInterface == NULL) {
            throw DataFileException(filename
                                    + " cannot be written because no file is loaded");
        }
        CiftiFile* ciftiFile = dynamic_cast<CiftiFile*>(m_ciftiInterface.getPointer());
        if (ciftiFile == NULL) {
            throw DataFileException(filename
                                    + " cannot be written because it was not read from a disk file and was"
                                    + " likely read via the network.");
        }
        
        if (getDataFileType() == DataFileTypeEnum::CONNECTIVITY_DENSE) {
            throw DataFileException(filename
                                    + " dense connectivity files cannot be written to files due to their large sizes.");
        }
        
        /*
         * Update the file's metadata.
         */
        m_ciftiFacade->setFileMetadata(m_metadata);
        
        /*
         * Update all data in the file.
         */
        const int32_t numMaps = getNumberOfMaps();
        for (int32_t i = 0; i < numMaps; i++) {
            /*
             * Does file have map attributes
             */
            if (hasMapAttributes()) {
                /*
                 * Replace the map's metadata.
                 */
                m_ciftiFacade->setMetadataForMapOrSeriesIndex(i, getMapMetaData(i));
            }
            
        }
        
        ciftiFile->writeFile(filename);
    }
    catch (const CiftiFileException& cfe) {
        throw DataFileException(cfe);
    }
}

/**
 * @return The string name of the CIFTI index type.
 * @param ciftiIndexType
 */
AString
CiftiMappableDataFile::ciftiIndexTypeToName(const IndicesMapToDataType ciftiIndexType)
{
    AString name = "Invalid";
    
    switch (ciftiIndexType) {
        case CIFTI_INDEX_TYPE_BRAIN_MODELS:
            name = "CIFTI_INDEX_TYPE_BRAIN_MODELS";
            break;
        case CIFTI_INDEX_TYPE_FIBERS:
            name = "CIFTI_INDEX_TYPE_FIBERS";
            break;
        case CIFTI_INDEX_TYPE_INVALID:
            name = "CIFTI_INDEX_TYPE_INVALID";
            break;
        case CIFTI_INDEX_TYPE_LABELS:
            name = "CIFTI_INDEX_TYPE_LABELS";
            break;
        case CIFTI_INDEX_TYPE_PARCELS:
            name = "CIFTI_INDEX_TYPE_PARCELS";
            break;
        case CIFTI_INDEX_TYPE_SCALARS:
            name = "CIFTI_INDEX_TYPE_SCALARS";
            break;
        case CIFTI_INDEX_TYPE_TIME_POINTS:
            name = "CIFTI_INDEX_TYPE_TIME_POINTS";
            break;
    }
    
    return name;
}


/**
 * @return Is the data mappable to a surface?
 */
bool
CiftiMappableDataFile::isSurfaceMappable() const
{
    return m_ciftiFacade->containsSurfaceDataForMappingToBrainordinates();
}

/**
 * @return Is the data mappable to a volume?
 */
bool
CiftiMappableDataFile::isVolumeMappable() const
{
    return m_containsVolumeData;
}

/**
 * @return The number of maps in the file.
 * Note: Caret5 used the term 'columns'.
 */
int32_t
CiftiMappableDataFile::getNumberOfMaps() const
{
    return m_mapContent.size();
}

/**
 * @return True if the file has map attributes (name and metadata).
 * For files that do not have map attributes, they should override
 * this method and return false.  If not overriden, this method
 * returns true.
 *
 * Some files (such as CIFTI Connectivity Matrix Files and CIFTI
 * Data-Series Files) do not have Map Attributes and thus there
 * is no map name nor map metadata and options to edit these
 * attributes should not be presented to the user.
 *
 * These CIFTI files do contain palette color mapping but it is
 * associated with the file.  To simplify palette color mapping editing
 * these file will return the file's palette color mapping for any
 * calls to getMapPaletteColorMapping().
 */
bool
CiftiMappableDataFile::hasMapAttributes() const
{
    if (m_ciftiFacade != NULL) {
        return m_ciftiFacade->containsMapAttributes();
    }
    
    return false;
}

/**
 * Get the name of the map at the given index.
 *
 * @param mapIndex
 *    Index of the map.
 * @return
 *    Name of the map.
 */
AString
CiftiMappableDataFile::getMapName(const int32_t mapIndex) const
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    return m_ciftiFacade->getNameForMapOrSeriesIndex(mapIndex);
}

/**
 * Set the name of the map at the given index.
 *
 * @param mapIndex
 *    Index of the map.
 * @param mapName
 *    New name for the map.
 */
void
CiftiMappableDataFile::setMapName(const int32_t mapIndex,
                                   const AString& mapName)
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    
    /*
     * If map name does not change, then get out
     */
    if (mapName == getMapName(mapIndex)) {
        return;
    }
    
    m_ciftiFacade->setNameForMapOrSeriesIndex(mapIndex,
                                              mapName);
    
    setModified();
}

/**
 * Get the metadata for the map at the given index
 *
 * @param mapIndex
 *    Index of the map.
 * @return
 *    Metadata for the map (const value).
 */
const GiftiMetaData*
CiftiMappableDataFile::getMapMetaData(const int32_t mapIndex) const
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    return m_mapContent[mapIndex]->m_metadata;
}

/**
 * Get the metadata for the map at the given index
 *
 * @param mapIndex
 *    Index of the map.
 * @return
 *    Metadata for the map.
 */
GiftiMetaData*
CiftiMappableDataFile::getMapMetaData(const int32_t mapIndex)
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    return m_mapContent[mapIndex]->m_metadata;
}

/**
 * Get the unique ID (UUID) for the map at the given index.
 *
 * @param mapIndex
 *    Index of the map.
 * @return
 *    String containing UUID for the map.
 */
AString
CiftiMappableDataFile::getMapUniqueID(const int32_t mapIndex) const
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    
    const GiftiMetaData* md = getMapMetaData(mapIndex);
    const AString uniqueID = md->getUniqueID();
    return uniqueID;
}

/**
 * @return Is the data in the file mapped to colors using
 * a palette.
 */
bool
CiftiMappableDataFile::isMappedWithPalette() const
{
    return m_ciftiFacade->isBrainordinateDataColoredWithPalette();
}

/**
 * Get the data for the given map index.
 *
 * @param mapIndex
 *     Index of the map.
 * @param dataOut
 *     A vector that will contain the data for the map upon exit.
 */
void
CiftiMappableDataFile::getMapData(const int32_t mapIndex,
                                  std::vector<float>& dataOut) const
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    m_ciftiFacade->getDataForMapOrSeriesIndex(mapIndex,
                                              dataOut);
}


/**
 * Get statistics describing the distribution of data
 * mapped with a color palette at the given index.
 *
 * @param mapIndex
 *    Index of the map.
 * @return
 *    Descriptive statistics for data (will be NULL for data
 *    not mapped using a palette).
 */
const DescriptiveStatistics*
CiftiMappableDataFile::getMapStatistics(const int32_t mapIndex)
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    
    std::vector<float> data;
    getMapData(mapIndex,
               data);
    
    DescriptiveStatistics* ds = m_mapContent[mapIndex]->m_descriptiveStatistics;
    if (data.empty()) {
        ds->update(NULL,
                   0);
    }
    else {
        ds->update(&data[0],
                   data.size());
    }
    return ds;
}

/**
 * Get statistics describing the distribution of data
 * mapped with a color palette at the given index.
 *
 * @param mapIndex
 *    Index of the map.
 * @return
 *    Fast statistics for data (will be NULL for data
 *    not mapped using a palette).
 */
const FastStatistics*
CiftiMappableDataFile::getMapFastStatistics(const int32_t mapIndex)
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    
    std::vector<float> data;
    getMapData(mapIndex,
               data);
    
    FastStatistics* fs = m_mapContent[mapIndex]->m_fastStatistics;
    if (data.empty()) {
        fs->update(NULL,
                   0);
    }
    else {
        fs->update(&data[0],
                   data.size());
    }
    return fs;
}

/**
 * Get histogram describing the distribution of data
 * mapped with a color palette at the given index.
 *
 * @param mapIndex
 *    Index of the map.
 * @return
 *    Histogram for data (will be NULL for data
 *    not mapped using a palette).
 */
const Histogram*
CiftiMappableDataFile::getMapHistogram(const int32_t mapIndex)
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);

    std::vector<float> data;
    getMapData(mapIndex,
               data);
    
    Histogram* h = m_mapContent[mapIndex]->m_histogram;
    if (data.empty()) {
        h->update(NULL,
                  0);
    }
    else {
        h->update(&data[0],
                  data.size());
    }
    return h;
}

/**
 * Get statistics describing the distribution of data
 * mapped with a color palette at the given index for
 * data within the given ranges.
 *
 * @param mapIndex
 *    Index of the map.
 * @param mostPositiveValueInclusive
 *    Values more positive than this value are excluded.
 * @param leastPositiveValueInclusive
 *    Values less positive than this value are excluded.
 * @param leastNegativeValueInclusive
 *    Values less negative than this value are excluded.
 * @param mostNegativeValueInclusive
 *    Values more negative than this value are excluded.
 * @param includeZeroValues
 *    If true zero values (very near zero) are included.
 * @return
 *    Descriptive statistics for data (will be NULL for data
 *    not mapped using a palette).
 */
const DescriptiveStatistics*
CiftiMappableDataFile::getMapStatistics(const int32_t mapIndex,
                                         const float mostPositiveValueInclusive,
                                         const float leastPositiveValueInclusive,
                                         const float leastNegativeValueInclusive,
                                         const float mostNegativeValueInclusive,
                                         const bool includeZeroValues)
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    
    std::vector<float> data;
    getMapData(mapIndex,
               data);
    
    
    DescriptiveStatistics* ds = m_mapContent[mapIndex]->m_descriptiveStatistics;
    if (data.empty()) {
        ds->update(NULL,
                   0);
    }
    else {
        ds->update(&data[0],
                   data.size(),
                   mostPositiveValueInclusive,
                   leastPositiveValueInclusive,
                   leastNegativeValueInclusive,
                   mostNegativeValueInclusive,
                   includeZeroValues);
    }
    return ds;
}

/**
 * Get histogram describing the distribution of data
 * mapped with a color palette at the given index for
 * data within the given ranges.
 *
 * @param mapIndex
 *    Index of the map.
 * @param mostPositiveValueInclusive
 *    Values more positive than this value are excluded.
 * @param leastPositiveValueInclusive
 *    Values less positive than this value are excluded.
 * @param leastNegativeValueInclusive
 *    Values less negative than this value are excluded.
 * @param mostNegativeValueInclusive
 *    Values more negative than this value are excluded.
 * @param includeZeroValues
 *    If true zero values (very near zero) are included.
 * @return
 *    Descriptive statistics for data (will be NULL for data
 *    not mapped using a palette).
 */
const Histogram*
CiftiMappableDataFile::getMapHistogram(const int32_t mapIndex,
                                        const float mostPositiveValueInclusive,
                                        const float leastPositiveValueInclusive,
                                        const float leastNegativeValueInclusive,
                                        const float mostNegativeValueInclusive,
                                        const bool includeZeroValues)
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);

    std::vector<float> data;
    getMapData(mapIndex,
               data);
    
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    Histogram* h = m_mapContent[mapIndex]->m_histogram;
    if (data.empty()) {
        h->update(NULL,
                  0);
    }
    else {
        h->update(&data[0],
                  data.size(),
                  mostPositiveValueInclusive,
                  leastPositiveValueInclusive,
                  leastNegativeValueInclusive,
                  mostNegativeValueInclusive,
                  includeZeroValues);
    }
    return h;
}

/**
 * Get the palette color mapping for the map at the given index.
 *
 * @param mapIndex
 *    Index of the map.
 * @return
 *    Palette color mapping for the map (will be NULL for data
 *    not mapped using a palette).
 */
PaletteColorMapping*
CiftiMappableDataFile::getMapPaletteColorMapping(const int32_t mapIndex)
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    if (hasMapAttributes()) {
        return m_mapContent[mapIndex]->m_paletteColorMapping;
    }
    
    CaretAssert(m_ciftiInterface);
    const CiftiXML& ciftiXML = m_ciftiInterface->getCiftiXML();
    return ciftiXML.getFilePalette();
}

/**
 * Get the palette color mapping for the map at the given index.
 *
 * @param mapIndex
 *    Index of the map.
 * @return
 *    Palette color mapping for the map (constant) (will be NULL for data
 *    not mapped using a palette).
 */
const PaletteColorMapping*
CiftiMappableDataFile::getMapPaletteColorMapping(const int32_t mapIndex) const
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    if (hasMapAttributes()) {
        return m_mapContent[mapIndex]->m_paletteColorMapping;
    }
    
    CaretAssert(m_ciftiInterface);
    const CiftiXML& ciftiXML = m_ciftiInterface->getCiftiXML();
    return ciftiXML.getFilePalette();
//    CaretAssertVectorIndex(m_mapContent,
//                           mapIndex);
//    
//    if (m_ciftiFacade->isConnectivityMatrixFile()) {
//        CaretAssert(m_ciftiInterface);
//        const CiftiXML& ciftiXML = m_ciftiInterface->getCiftiXML();
//        return ciftiXML.getFilePalette();
//    }
//
//    return m_mapContent[mapIndex]->m_paletteColorMapping;
}

/**
 * @return Is the data in the file mapped to colors using
 * a label table.
 */
bool
CiftiMappableDataFile::isMappedWithLabelTable() const
{
    return m_ciftiFacade->isBrainordinateDataColoredWithLabelTable();
}

/**
 * Get the label table for the map at the given index.
 *
 * @param mapIndex
 *    Index of the map.
 * @return
 *    Label table for the map (will be NULL for data
 *    not mapped using a label table).
 */
GiftiLabelTable*
CiftiMappableDataFile::getMapLabelTable(const int32_t mapIndex)
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    return m_mapContent[mapIndex]->m_labelTable;
}

/**
 * Get the label table for the map at the given index.
 *
 * @param mapIndex
 *    Index of the map.
 * @return
 *    Label table for the map (constant) (will be NULL for data
 *    not mapped using a label table).
 */
const GiftiLabelTable*
CiftiMappableDataFile::getMapLabelTable(const int32_t mapIndex) const
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    return m_mapContent[mapIndex]->m_labelTable;
}

/**
 * Update scalar coloring for a map.
 *
 * @param mapIndex
 *    Index of map.
 * @param paletteFile
 *    Palette file containing palettes.
 */
void
CiftiMappableDataFile::updateScalarColoringForMap(const int32_t mapIndex,
                                                   const PaletteFile* paletteFile)
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    std::vector<float> data;
    getMapData(mapIndex,
               data);

    if (m_ciftiFacade->isBrainordinateDataColoredWithLabelTable()) {
        m_mapContent[mapIndex]->updateColoring(data,
                                               NULL);
    }
    else if (m_ciftiFacade->isBrainordinateDataColoredWithPalette()) {
        m_mapContent[mapIndex]->updateColoring(data,
                                               paletteFile);
    }
    else {
        CaretAssert(0);
    }
}

/**
 * Get the dimensions of the volume.
 *
 * @param dimOut1
 *     First dimension (i) out.
 * @param dimOut2
 *     Second dimension (j) out.
 * @param dimOut3
 *     Third dimension (k) out.
 * @param dimTimeOut
 *     Time dimensions out (number of maps)
 * @param numComponentsOut
 *     Number of components per voxel.
 */
void
CiftiMappableDataFile::getDimensions(int64_t& dimOut1,
                           int64_t& dimOut2,
                           int64_t& dimOut3,
                           int64_t& dimTimeOut,
                           int64_t& numComponentsOut) const
{
    dimOut1 = m_volumeDimensions[0];
    dimOut2 = m_volumeDimensions[1];
    dimOut3 = m_volumeDimensions[2];
    dimTimeOut = m_volumeDimensions[3];
    numComponentsOut = m_volumeDimensions[4];
}

/**
 * Get the dimensions of the volume.
 *
 * @param dimsOut
 *     Will contain 5 elements: (0) X-dimension, (1) Y-dimension
 * (2) Z-dimension, (3) time, (4) components.
 */
void
CiftiMappableDataFile::getDimensions(std::vector<int64_t>& dimsOut) const
{
    dimsOut.push_back(m_volumeDimensions[0]);
    dimsOut.push_back(m_volumeDimensions[1]);
    dimsOut.push_back(m_volumeDimensions[2]);
    dimsOut.push_back(m_volumeDimensions[3]);
    dimsOut.push_back(m_volumeDimensions[4]);
}

/**
 * @return The number of componenents per voxel in the volume data.
 */
const int64_t&
CiftiMappableDataFile::getNumberOfComponents() const
{
    return m_volumeDimensions[4];
}

/**
 * Convert an index to space (coordinates).
 *
 * @param indexIn1
 *     First dimension (i).
 * @param indexIn2
 *     Second dimension (j).
 * @param indexIn3
 *     Third dimension (k).
 * @param coordOut1
 *     Output first (x) coordinate.
 * @param coordOut2
 *     Output first (y) coordinate.
 * @param coordOut3
 *     Output first (z) coordinate.
 */
void
CiftiMappableDataFile::indexToSpace(const float& indexIn1,
                          const float& indexIn2,
                          const float& indexIn3,
                          float& coordOut1,
                          float& coordOut2,
                          float& coordOut3) const
{
    CaretAssert(m_voxelIndicesToOffset);
    m_voxelIndicesToOffset->indicesToCoordinate(indexIn1,
                                                indexIn2,
                                                indexIn3,
                                                coordOut1,
                                                coordOut2,
                                                coordOut3);
}

/**
 * Convert an index to space (coordinates).
 *
 * @param indexIn1
 *     First dimension (i).
 * @param indexIn2
 *     Second dimension (j).
 * @param indexIn3
 *     Third dimension (k).
 * @param coordOut
 *     Output XYZ coordinates.
 */
void
CiftiMappableDataFile::indexToSpace(const float& indexIn1,
                          const float& indexIn2,
                          const float& indexIn3,
                          float* coordOut) const
{
    CaretAssert(m_voxelIndicesToOffset);
    m_voxelIndicesToOffset->indicesToCoordinate(indexIn1,
                                                indexIn2,
                                                indexIn3,
                                                coordOut[0],
                                                coordOut[1],
                                                coordOut[2]);
}

/**
 * Convert an index to space (coordinates).
 *
 * @param indexIn
 *     IJK indices
 * @param coordOut
 *     Output XYZ coordinates.
 */
void
CiftiMappableDataFile::indexToSpace(const int64_t* indexIn,
                          float* coordOut) const
{
    CaretAssert(m_voxelIndicesToOffset);
    m_voxelIndicesToOffset->indicesToCoordinate(indexIn[0],
                                                indexIn[1],
                                                indexIn[2],
                                                coordOut[0],
                                                coordOut[1],
                                                coordOut[2]);
}

/**
 * Convert a coordinate to indices.  Note that output indices
 * MAY NOT BE WITHIN THE VALID VOXEL DIMENSIONS.
 *
 * @param coordIn1
 *     First (x) input coordinate.
 * @param coordIn2
 *     Second (y) input coordinate.
 * @param coordIn3
 *     Third (z) input coordinate.
 * @param indexOut1
 *     First output index (i).
 * @param indexOut2
 *     First output index (j).
 * @param indexOut3
 *     First output index (k).
 */
void
CiftiMappableDataFile::enclosingVoxel(const float& coordIn1,
                                      const float& coordIn2,
                                      const float& coordIn3,
                                      int64_t& indexOut1,
                                      int64_t& indexOut2,
                                      int64_t& indexOut3) const
{
    CaretAssert(m_voxelIndicesToOffset);
    m_voxelIndicesToOffset->coordinateToIndices(coordIn1,
                                                coordIn2,
                                                coordIn3,
                                                indexOut1,
                                                indexOut2,
                                                indexOut3);
}

/**
 * Determine in the given voxel indices are valid (within the volume dimensions).
 *
 * @param indexIn1
 *     First dimension (i).
 * @param indexIn2
 *     Second dimension (j).
 * @param indexIn3
 *     Third dimension (k).
 * @param coordOut1
 *     Output first (x) coordinate.
 * @param brickIndex
 *     Time/map index (default 0).
 * @param component
 *     Voxel component (default 0).
 */
bool
CiftiMappableDataFile::indexValid(const int64_t& indexIn1,
                        const int64_t& indexIn2,
                        const int64_t& indexIn3,
                        const int64_t /*brickIndex*/,
                        const int64_t /*component*/) const
{
    if ((indexIn1 >= 0)
        && (indexIn1 < m_volumeDimensions[0])
        && (indexIn2 >= 0)
        && (indexIn2 < m_volumeDimensions[1])
        && (indexIn3 >= 0)
        && (indexIn3 < m_volumeDimensions[2])) {
        return true;
    }
    
    return false;
}

/**
 * Get a bounding box for the voxel coordinate ranges.
 *
 * @param boundingBoxOut
 *    The output bounding box.
 */
void
CiftiMappableDataFile::getVoxelSpaceBoundingBox(BoundingBox& boundingBoxOut) const
{
    CaretAssert(m_voxelIndicesToOffset);
    
    boundingBoxOut.resetForUpdate();
    
    if (m_voxelIndicesToOffset->isValid()) {
        float xyz[3];
        indexToSpace(0,
                     0,
                     0,
                     xyz);
        boundingBoxOut.update(xyz);
        
        indexToSpace(m_volumeDimensions[0] - 1,
                     m_volumeDimensions[1] - 1,
                     m_volumeDimensions[2] - 1,
                     xyz);
        
        boundingBoxOut.update(xyz);
    }
}

/**
 * Get the voxel colors for a slice in the map.
 *
 * @param mapIndex
 *    Index of the map.
 * @param slicePlane
 *    The slice plane.
 * @param sliceIndex
 *    Index of the slice.
 * @param rgbaOut
 *    Output containing the rgba values (must have been allocated
 *    by caller to sufficient count of elements in the slice).
 */
void
CiftiMappableDataFile::getVoxelColorsForSliceInMap(const int32_t mapIndex,
                                         const VolumeSliceViewPlaneEnum::Enum slicePlane,
                                         const int64_t sliceIndex,
                                         uint8_t* rgbaOut) const
{
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    CaretAssertMessage((sliceIndex >= 0),
                       "Slice index is invalid.");
    if (sliceIndex < 0) {
        return;
    }
    
    const int64_t dimI = m_volumeDimensions[0];
    const int64_t dimJ = m_volumeDimensions[1];
    const int64_t dimK = m_volumeDimensions[2];
    
    int64_t voxelCount = 0;
    
    switch (slicePlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            CaretAssert(0);
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            voxelCount = dimI * dimJ;
            CaretAssert((sliceIndex < dimK));
            if (sliceIndex >= dimK) {
                return;
            }
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            voxelCount = dimI * dimK;
            CaretAssert((sliceIndex < dimJ));
            if (sliceIndex >= dimJ) {
                return;
            }
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            voxelCount = dimJ * dimK;
            CaretAssert((sliceIndex < dimI));
            if (sliceIndex >= dimI) {
                return;
            }
            break;
    }
    
    if (voxelCount <= 0) {
        return;
    }
    const int64_t componentCount = voxelCount * 4;
    
    /*
     * Clear the slice rgba coloring.
     */
    for (int64_t i = 0; i < componentCount; i++) {
        rgbaOut[i] = 0;
    }
    
    const int64_t mapRgbaCount = m_mapContent[mapIndex]->m_rgba.size();
    CaretAssert(mapRgbaCount > 0);
    if (mapRgbaCount <= 0) {
        return;
    }
    
    const float* mapRGBA = &m_mapContent[mapIndex]->m_rgba[0];
    
    CaretAssert(m_voxelIndicesToOffset);
    
    /*
     * Set the rgba components for the slice.
     */
    switch (slicePlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            CaretAssert(0);
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            for (int64_t j = 0; j < dimJ; j++) {
                for (int64_t i = 0; i < dimI; i++) {
                    const int64_t dataOffset = m_voxelIndicesToOffset->getOffsetForIndices(i,
                                                                                           j,
                                                                                           sliceIndex);
                    if (dataOffset >= 0) {
                        const int64_t dataOffset4 = dataOffset * 4;
                        CaretAssert(dataOffset4 < mapRgbaCount);
                        
                        const int64_t rgbaOffset = ((j * dimI) + i) * 4;
                        CaretAssert(rgbaOffset < componentCount);
                        rgbaOut[rgbaOffset]   = (mapRGBA[dataOffset4] * 255.0);
                        rgbaOut[rgbaOffset+1] = (mapRGBA[dataOffset4+1] * 255.0);
                        rgbaOut[rgbaOffset+2] = (mapRGBA[dataOffset4+2] * 255.0);
                        rgbaOut[rgbaOffset+3] = (mapRGBA[dataOffset4+3] * 255.0);
                    }
                }
            }
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            for (int64_t k = 0; k < dimK; k++) {
                for (int64_t i = 0; i < dimI; i++) {
                    const int64_t dataOffset = m_voxelIndicesToOffset->getOffsetForIndices(i,
                                                                                           sliceIndex,
                                                                                           k);
                    if (dataOffset >= 0) {
                        const int64_t dataOffset4 = dataOffset * 4;
                        CaretAssert(dataOffset4 < mapRgbaCount);
                        
                        const int64_t rgbaOffset = ((k * dimI) + i) * 4;
                        CaretAssert(rgbaOffset < componentCount);
                        rgbaOut[rgbaOffset]   = (mapRGBA[dataOffset4] * 255.0);
                        rgbaOut[rgbaOffset+1] = (mapRGBA[dataOffset4+1] * 255.0);
                        rgbaOut[rgbaOffset+2] = (mapRGBA[dataOffset4+2] * 255.0);
                        rgbaOut[rgbaOffset+3] = (mapRGBA[dataOffset4+3] * 255.0);
                    }
                }
            }
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            for (int64_t k = 0; k < dimK; k++) {
                for (int64_t j = 0; j < dimJ; j++) {
                    const int64_t dataOffset = m_voxelIndicesToOffset->getOffsetForIndices(sliceIndex,
                                                                                           j,
                                                                                           k);
                    if (dataOffset >= 0) {
                        const int64_t dataOffset4 = dataOffset * 4;
                        CaretAssert(dataOffset4 < mapRgbaCount);
                        
                        const int64_t rgbaOffset = ((k * dimJ) + j) * 4;
                        CaretAssert(rgbaOffset < componentCount);
                        rgbaOut[rgbaOffset]   = (mapRGBA[dataOffset4] * 255.0);
                        rgbaOut[rgbaOffset+1] = (mapRGBA[dataOffset4+1] * 255.0);
                        rgbaOut[rgbaOffset+2] = (mapRGBA[dataOffset4+2] * 255.0);
                        rgbaOut[rgbaOffset+3] = (mapRGBA[dataOffset4+3] * 255.0);
                    }
                }
            }
            break;
    }
}

/**
 * Get the voxel coloring for the voxel at the given indices.
 *
 * @param indexIn1
 *     First dimension (i).
 * @param indexIn2
 *     Second dimension (j).
 * @param indexIn3
 *     Third dimension (k).
 * @param mapIndex
 *     Time/map index.
 * @param rgbaOut
 *     Output containing RGBA values for voxel at the given indices.
 */
void
CiftiMappableDataFile::getVoxelColorInMap(const int64_t indexIn1,
                                const int64_t indexIn2,
                                const int64_t indexIn3,
                                const int64_t mapIndex,
                                uint8_t rgbaOut[4]) const
{
    rgbaOut[0] = 0;
    rgbaOut[1] = 0;
    rgbaOut[2] = 0;
    rgbaOut[3] = 0;
    
    const int64_t mapRgbaCount = m_mapContent[mapIndex]->m_rgba.size();
    CaretAssert(mapRgbaCount > 0);
    if (mapRgbaCount <= 0) {
        return;
    }
    
    CaretAssert(m_voxelIndicesToOffset);
    
    const float* mapRGBA = &m_mapContent[mapIndex]->m_rgba[0];
    const int64_t dataOffset = m_voxelIndicesToOffset->getOffsetForIndices(indexIn1,
                                                                           indexIn2,
                                                                           indexIn3);
    if (dataOffset >= 0) {
        const int64_t dataOffset4 = dataOffset * 4;
        CaretAssert(dataOffset4 < mapRgbaCount);
        
        rgbaOut[0] = (mapRGBA[dataOffset4] * 255.0);
        rgbaOut[1] = (mapRGBA[dataOffset4+1] * 255.0);
        rgbaOut[2] = (mapRGBA[dataOffset4+2] * 255.0);
        rgbaOut[3] = (mapRGBA[dataOffset4+3] * 255.0);
    }
}

/**
 * Get the unique label keys in the given map.
 * @param mapIndex
 *    Index of the map.
 * @return
 *    Keys used by the map.
 */
std::vector<int32_t>
CiftiMappableDataFile::getUniqueLabelKeysUsedInMap(const int32_t mapIndex) const
{
    CaretAssertVectorIndex(m_mapContent, mapIndex);
    
    std::vector<float> data;
    getMapData(mapIndex,
               data);
    std::set<int32_t> uniqueKeys;
    const int64_t numItems = static_cast<int64_t>(data.size());
    if (numItems > 0) {
        const float* dataPtr = &data[0];
        for (int64_t i = 0; i < numItems; i++) {
            const int32_t key = static_cast<int32_t>(dataPtr[i]);
            uniqueKeys.insert(key);
        }
    }
    
    std::vector<int32_t> keyVector;
    keyVector.insert(keyVector.end(),
                     uniqueKeys.begin(),
                     uniqueKeys.end());
    return keyVector;
}

/**
 * @return The class and name hierarchy.
 */
GroupAndNameHierarchyModel*
CiftiMappableDataFile::getGroupAndNameHierarchyModel()
{
    CaretAssert(m_classNameHierarchy);
    
    m_classNameHierarchy->update(this,
                                 m_forceUpdateOfGroupAndNameHierarchy);
    m_forceUpdateOfGroupAndNameHierarchy = false;
    
    return m_classNameHierarchy;
}

/**
 * @return The class and name hierarchy.
 */
const GroupAndNameHierarchyModel*
CiftiMappableDataFile::getGroupAndNameHierarchyModel() const
{
    CaretAssert(m_classNameHierarchy);
    
    m_classNameHierarchy->update(const_cast<CiftiMappableDataFile*>(this),
                                 m_forceUpdateOfGroupAndNameHierarchy);
    m_forceUpdateOfGroupAndNameHierarchy = false;
    
    return m_classNameHierarchy;
}

/**
 * Validate keys and labels in the file.
 */
void
CiftiMappableDataFile::validateKeysAndLabels() const
{
    /*
     * Skip if logging is not fine or less.
     */
    if (CaretLogger::getLogger()->isFine() == false) {
        return;
    }
    
    AString messages;
    
    /*
     * Find the label keys that are in the data
     */
    std::set<int32_t> dataKeys;
    const int32_t numMaps  = getNumberOfMaps();
    for (int32_t jMap = 0; jMap < numMaps; jMap++) {
        AString mapMessage;
        
        std::vector<float> data;
        getMapData(jMap,
                   data);
        const int64_t numItems = static_cast<int64_t>(data.size());
        for (int32_t i = 0; i < numItems; i++) {
            const int32_t key = static_cast<int32_t>(data[i]);
            dataKeys.insert(key);
        }
        
        /*
         * Find any keys that are not in the label table
         */
        const GiftiLabelTable* labelTable = getMapLabelTable(jMap);
        std::set<int32_t> missingLabelKeys;
        for (std::set<int32_t>::iterator dataKeyIter = dataKeys.begin();
             dataKeyIter != dataKeys.end();
             dataKeyIter++) {
            const int32_t dataKey = *dataKeyIter;
            
            const GiftiLabel* label = labelTable->getLabel(dataKey);
            if (label == NULL) {
                missingLabelKeys.insert(dataKey);
            }
        }
        
        if (missingLabelKeys.empty() == false) {
            for (std::set<int32_t>::iterator missingKeyIter = missingLabelKeys.begin();
                 missingKeyIter != missingLabelKeys.end();
                 missingKeyIter++) {
                const int32_t missingKey = *missingKeyIter;
                
                mapMessage.appendWithNewLine("        Missing Label for Key: "
                                             + AString::number(missingKey));
            }
        }
        
        /*
         * Find any label table names that are not used
         */
        std::map<int32_t, AString> labelTableKeysAndNames;
        labelTable->getKeysAndNames(labelTableKeysAndNames);
        for (std::map<int32_t, AString>::const_iterator ltIter = labelTableKeysAndNames.begin();
             ltIter != labelTableKeysAndNames.end();
             ltIter++) {
            const int32_t ltKey = ltIter->first;
            if (std::find(dataKeys.begin(),
                          dataKeys.end(),
                          ltKey) == dataKeys.end()) {
                mapMessage.appendWithNewLine("        Label Not Used Key="
                                             + AString::number(ltKey)
                                             + ": "
                                             + ltIter->second);
            }
        }
        
        if (mapMessage.isEmpty() == false) {
            mapMessage = ("    Map: "
                          + getMapName(jMap)
                          + ":\n"
                          + mapMessage
                          + "\n"
                          + labelTable->toFormattedString("        "));
            messages += mapMessage;
        }
    }
    
    
    AString msg = ("File: "
                   + getFileName()
                   + "\n"
                   + messages);
    CaretLogFine(msg);
}

/**
 * @return true if the CIFTI interface is valid, else false.
 * A message is logged if the interface is not valid.
 */
bool
CiftiMappableDataFile::isCiftiInterfaceValid() const
{
    if (m_ciftiInterface != NULL) {
        return true;
    }

    CaretLogSevere(getFileNameNoPath()
                   + "\" of type\""
                   + DataFileTypeEnum::toName(getDataFileType())
                   + "\" does not have a file loaded (CIFTI Interface is not valid).");
    return false;
}


/**
 * Get connectivity value for a surface's node.  When the data is mapped
 * to parcels, the numerical value will not be valid.  
 
 * @param structure
 *     Surface's structure.
 * @param nodeIndex
 *     Index of the node
 * @param numberOfNodes
 *     Number of nodes in the surface.
 * @param numericalValueOut
 *     Numerical value out.
 * @param numericalValueOutValid
 *     Output that indicates the numerical value output is valid.
 * @param textValueOut
 *     Text containing node' value will always be valid if the method 
 *     returns true.
 * @return
 *    True if the text value is valid.  The numerical value may or may not
 *    also be valid.
 */
bool
CiftiMappableDataFile::getMapSurfaceNodeValue(const int32_t mapIndex,
                                              const StructureEnum::Enum structure,
                                              const int nodeIndex,
                                              const int32_t numberOfNodes,
                                              float& numericalValueOut,
                                              bool& numericalValueOutValid,
                                              AString& textValueOut) const
{
    numericalValueOut = false;
    
    if (isCiftiInterfaceValid() == false) {
        return false;
    }
    
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);

    const CiftiXML& ciftiXML = m_ciftiInterface->getCiftiXML();
    
    int32_t numCiftiNodes = -1;
    
    /*
     * Validate number of nodes are correct
     */
    switch (m_brainordinateMappedDataAccess) {
        case DATA_ACCESS_INVALID:
            break;
        case DATA_ACCESS_WITH_COLUMN_METHODS:
            numCiftiNodes = m_ciftiInterface->getColumnSurfaceNumberOfNodes(structure);
            break;
        case DATA_ACCESS_WITH_ROW_METHODS:
            numCiftiNodes = m_ciftiInterface->getRowSurfaceNumberOfNodes(structure);
            break;
    }
    
    if (numCiftiNodes != numberOfNodes) {
        return false;
    }
    
    if (m_ciftiFacade->isMappingDataToBrainordinateParcels()) {
        /*
         * Get content for map.
         */
        int64_t parcelMapIndex = -1;
        std::vector<CiftiParcelElement> parcels;
        switch (m_brainordinateMappedDataAccess) {
            case DATA_ACCESS_INVALID:
                break;
            case DATA_ACCESS_WITH_COLUMN_METHODS:
                ciftiXML.getParcelsForColumns(parcels);
                parcelMapIndex = ciftiXML.getColumnParcelForNode(nodeIndex,
                                                                 structure);
                break;
            case DATA_ACCESS_WITH_ROW_METHODS:
                ciftiXML.getParcelsForRows(parcels);
                parcelMapIndex = ciftiXML.getRowParcelForNode(nodeIndex,
                                                              structure);
                break;
        }
        
        if ((parcelMapIndex >= 0)
            && (parcelMapIndex < static_cast<int64_t>(parcels.size()))) {
            textValueOut = parcels[parcelMapIndex].m_parcelName;
            return true;
        }
    }
    else {
        /*
         * Get content for map.
         */
        int64_t dataIndex = -1;
        std::vector<CiftiSurfaceMap> nodeMap;
        switch (m_brainordinateMappedDataAccess) {
            case DATA_ACCESS_INVALID:
                break;
            case DATA_ACCESS_WITH_COLUMN_METHODS:
                ciftiXML.getSurfaceMapForColumns(nodeMap,
                                                 structure);
                dataIndex = ciftiXML.getColumnIndexForNode(nodeIndex,
                                                           structure);
                break;
            case DATA_ACCESS_WITH_ROW_METHODS:
                ciftiXML.getSurfaceMapForRows(nodeMap,
                                              structure);
                dataIndex = ciftiXML.getRowIndexForNode(nodeIndex,
                                                        structure);
                break;
        }
        
        if (dataIndex >= 0) {
            std::vector<float> mapData;
            getMapData(mapIndex, mapData);
            
            if (dataIndex < static_cast<int64_t>(mapData.size())) {
                numericalValueOut = mapData[dataIndex];
                numericalValueOutValid = true;
                textValueOut = AString::number(numericalValueOut, 'f');
                return true;
            }
        }

        /*
         * At this time, ciftiXML.getRowIndexForNode() does not return
         * the index for a node in a Parcel Dense File.  
         */
        if (nodeMap.empty() == false) {
            const int64_t numNodeMaps = static_cast<int32_t>(nodeMap.size());
            for (int i = 0; i < numNodeMaps; i++) {
                if (nodeMap[i].m_surfaceNode == nodeIndex) {
                    std::vector<float> mapData;
                    getMapData(mapIndex, mapData);
                    
                    CaretAssertVectorIndex(mapData,
                                           nodeMap[i].m_ciftiIndex);
                    numericalValueOut = mapData[nodeMap[i].m_ciftiIndex];
                    numericalValueOutValid = true;
                    textValueOut = AString::number(numericalValueOut, 'f');
                    return true;
                }
            }
        }
    }
    
    return false;
}

/**
 * Get the identification information for a surface node in the given maps.
 *
 * @param mapIndices
 *    Indices of maps for which identification information is requested.
 * @param structure
 *    Structure of the surface.
 * @param nodeIndex
 *    Index of the node.
 * @param numberOfNodes
 *    Number of nodes in the surface.
 * @param textOut
 *    Output containing identification information.
 */
bool
CiftiMappableDataFile::getMapSurfaceNodeIdentificationForMaps(const std::vector<int32_t> mapIndices,
                                                              const StructureEnum::Enum structure,
                                                              const int nodeIndex,
                                                              const int32_t numberOfNodes,
                                                              AString& textOut) const
{
    if (isCiftiInterfaceValid() == false) {
        return false;
    }
    
    bool useMapData = false;
    bool useSeriesData = false;
    switch (m_ciftiFacade->getCiftiFileType()) {
        case CiftiFacade::CIFTI_INVALID:
            break;
        case CiftiFacade::CIFTI_DENSE:
            useMapData = true;
            break;
        case CiftiFacade::CIFTI_LABEL:
            useSeriesData = true;
            break;
        case CiftiFacade::CIFTI_SCALAR:
            useSeriesData = true;
            break;
        case CiftiFacade::CIFTI_DENSE_PARCEL:
            useMapData = true;
            break;
        case CiftiFacade::CIFTI_DATA_SERIES:
            useSeriesData = true;
            break;
        case CiftiFacade::CIFTI_PARCEL:
            useMapData = true;
            break;
        case CiftiFacade::CIFTI_PARCEL_DENSE:
            useMapData = true;
            break;
    }
    
    
    const int32_t numberOfMapIndices = static_cast<int32_t>(mapIndices.size());
    
    textOut = "";
    
    bool validID = false;
    
    if (useMapData) {
        for (int32_t i = 0; i < numberOfMapIndices; i++) {
            const int32_t mapIndex = mapIndices[i];
            
            float numericalValue;
            AString textValue;
            bool numericalValueValid;
            if (getMapSurfaceNodeValue(mapIndex,
                                       structure,
                                       nodeIndex,
                                       numberOfNodes,
                                       numericalValue,
                                       numericalValueValid,
                                       textValue)) {
                textOut += textValue;
                textOut += " ";
                validID = true;
            }
        }
    }
    else {
        /*
         * Use series data which contains values for node from all maps.
         */
        std::vector<float> seriesData;
        if (getSeriesDataForSurfaceNode(structure,
                                        nodeIndex,
                                        seriesData)) {
            for (int32_t i = 0; i < numberOfMapIndices; i++) {
                const int32_t mapIndex = mapIndices[i];
                CaretAssertVectorIndex(seriesData, mapIndex);

                const float value = seriesData[mapIndex];
                if (m_ciftiFacade->isBrainordinateDataColoredWithLabelTable()) {
                    const GiftiLabelTable* glt = getMapLabelTable(mapIndex);
                    const int32_t labelKey = static_cast<int32_t>(value);
                    const GiftiLabel* gl = glt->getLabel(labelKey);
                    if (gl != NULL) {
                        textOut += gl->getName();
                    }
                    else {
                        textOut += ("InvalidKey="
                                    + AString::number(value));
                    }
                    validID = true;
                }
                else if (m_ciftiFacade->isBrainordinateDataColoredWithPalette()) {
                    textOut += AString::number(value);
                    validID = true;
                }
                else {
                    CaretAssert(0);
                }
                
                textOut += " ";
            }
        }
    }
    
    return validID;
}

/**
 * Get the series data (one data value from each map) for a surface node.
 *
 * @param structure
 *     Surface's structure.
 * @param nodeIndex
 *     Index of the node.
 * @param seriesDataOut
 *     Series data for given node.
 * @return
 *     True if output data is valid, else false.
 */
bool
CiftiMappableDataFile::getSeriesDataForSurfaceNode(const StructureEnum::Enum structure,
                                         const int32_t nodeIndex,
                                         std::vector<float>& seriesDataOut) const
{
    if (isCiftiInterfaceValid() == false) {
        return false;
    }
    
    const bool valid = m_ciftiFacade->getSeriesDataForSurfaceNode(structure,
                                                                  nodeIndex,
                                                                  seriesDataOut);
    return valid;
}

/**
 * Get the node coloring for the surface.
 * @param surface
 *    Surface whose nodes are colored.
 * @param surfaceRGBAOut
 *    Filled with RGBA coloring for the surface's nodes.
 *    Contains numberOfNodes * 4 elements.
 * @param dataValuesOut
 *    Data values for the nodes (elements are valid when the alpha value in
 *    the RGBA colors is valid (greater than zero).
 * @param surfaceNumberOfNodes
 *    Number of nodes in the surface.
 * @return
 *    True if coloring is valid, else false.
 */
bool
CiftiMappableDataFile::getMapSurfaceNodeColoring(const int32_t mapIndex,
                                                  const StructureEnum::Enum structure,
                                                  float* surfaceRGBAOut,
                                                  float* dataValuesOut,
                                                  const int32_t surfaceNumberOfNodes)
{
    if (isCiftiInterfaceValid() == false) {
        return false;
    }
    
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    
    int32_t numCiftiNodes = -1;
    
    /*
     * Validate number of nodes are correct
     */
    switch (m_brainordinateMappedDataAccess) {
        case DATA_ACCESS_INVALID:
            break;
        case DATA_ACCESS_WITH_COLUMN_METHODS:
            numCiftiNodes = m_ciftiInterface->getColumnSurfaceNumberOfNodes(structure);
            break;
        case DATA_ACCESS_WITH_ROW_METHODS:
            numCiftiNodes = m_ciftiInterface->getRowSurfaceNumberOfNodes(structure);
            break;
    }
    
    if (numCiftiNodes != surfaceNumberOfNodes) {
        return false;
    }
    
    std::vector<float> mapData;
    getMapData(mapIndex,
               mapData);
    
    /*
     * Map data may be empty for connectivity matrix files with no rows loaded.
     */
    if (mapData.empty()) {
        return false;
    }
    
    const MapContent* mc = m_mapContent[mapIndex];
    std::vector<int64_t> dataIndicesForNodes;
    m_ciftiFacade->getSurfaceDataIndicesForMappingToBrainordinates(dataIndicesForNodes,
                                                                   structure,
                                                                   surfaceNumberOfNodes);

    bool validColorsFlag = false;
    
    for (int64_t iNode = 0; iNode < surfaceNumberOfNodes; iNode++) {
        CaretAssertVectorIndex(dataIndicesForNodes,
                               iNode);
        
        const int64_t dataIndex = dataIndicesForNodes[iNode];
        
        const int64_t node4 = iNode * 4;
        CaretAssertArrayIndex(surfaceRGBA, (surfaceNumberOfNodes * 4), node4);
        
        if (dataIndex > 0) {
            CaretAssert(dataIndex < mc->m_dataCount);
            
            const int64_t data4 = dataIndex * 4;
            CaretAssertArrayIndex(this->dataRGBA, (mc->m_dataCount * 4), data4);
            
            surfaceRGBAOut[node4]   = mc->m_rgba[data4];
            surfaceRGBAOut[node4+1] = mc->m_rgba[data4+1];
            surfaceRGBAOut[node4+2] = mc->m_rgba[data4+2];
            surfaceRGBAOut[node4+3] = mc->m_rgba[data4+3];
            
            dataValuesOut[iNode] = mapData[dataIndex];
         
            validColorsFlag = true;
        }
        else {
            surfaceRGBAOut[node4]   =  0.0;
            surfaceRGBAOut[node4+1] =  0.0;
            surfaceRGBAOut[node4+2] =  0.0;
            surfaceRGBAOut[node4+3] = -1.0;
            
            dataValuesOut[iNode] = 0.0;
        }
    }
    
    return validColorsFlag;
}

/**
 * Get connectivity value for a voxel at the given coordinate.
 * @param xyz
 *     Coordinate of voxel.
 * @param ijkOut
 *     Voxel indices of value.
 * @param textOut
 *     Text containing node value and for some types, the parcel.
 * @return
 *    true if a value was available for the voxel, else false.
 */
bool
CiftiMappableDataFile::getMapVolumeVoxelValue(const int32_t mapIndex,
                                               const float xyz[3],
                                               int64_t ijkOut[3],
                                               AString& textOut) const
{
    textOut = "";
    
    if (isCiftiInterfaceValid() == false) {
        return false;
    }
    
    /*
     * Get content for map.
     */
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    
    int64_t vfIJK[3];
    enclosingVoxel(xyz[0],
                   xyz[1],
                   xyz[2],
                   vfIJK[0],
                   vfIJK[1],
                   vfIJK[2]);
    if (indexValid(vfIJK[0],
                   vfIJK[1],
                   vfIJK[2])) {
        const int64_t dataOffset = m_voxelIndicesToOffset->getOffsetForIndices(vfIJK[0],
                                                                               vfIJK[1],
                                                                               vfIJK[2]);
        if (dataOffset >= 0) {
            std::vector<float> mapData;
            getMapData(mapIndex,
                       mapData);
            CaretAssertVectorIndex(mapData,
                                   dataOffset);
            const float value = mapData[dataOffset];
            
            if (m_ciftiFacade->isBrainordinateDataColoredWithLabelTable()) {
                textOut = "Invalid Label Index";
                
                const GiftiLabelTable* glt = getMapLabelTable(mapIndex);
                const int32_t labelKey = static_cast<int32_t>(value);
                const GiftiLabel* gl = glt->getLabel(labelKey);
                if (gl != NULL) {
                    textOut = gl->getName();
                }
            }
            else if (m_ciftiFacade->isBrainordinateDataColoredWithPalette()) {
                textOut = AString::number(value);
            }
            else {
                CaretAssert(0);
            }
            ijkOut[0] = vfIJK[0];
            ijkOut[1] = vfIJK[1];
            ijkOut[2] = vfIJK[2];
            
            return true;
        }
    }
    
    return false;
}

/**
 * Set the status to unmodified.
 */
void
CiftiMappableDataFile::clearModified()
{
    CaretMappableDataFile::clearModified();
    
    m_metadata->clearModified();
    
    const int32_t numMaps = getNumberOfMaps();
    for (int32_t i = 0; i < numMaps; i++) {
        m_mapContent[i]->clearModifiedStatus();
    }
}

/**
 * Is the object modified?
 * @return true if modified, else false.
 */
bool
CiftiMappableDataFile::isModified() const
{
    if (CaretMappableDataFile::isModified()) {
        return true;
    }
    
    if (m_metadata->isModified()) {
        return true;
    }
    
    const int32_t numMaps = getNumberOfMaps();
    for (int32_t i = 0; i < numMaps; i++) {
        if (m_mapContent[i]->isModifiedStatus()) {
            return true;
        }
    }
    
    return false;
}


/* ========================================================================== */

/**
 * Constructor.
 *
 * @param ciftiFacade
 *    CIFTI Facade that hides complexities of CIFTI interface.
 * @param mapIndex
 *    Index of this map.
 */
CiftiMappableDataFile::MapContent::MapContent(CiftiFacade* ciftiFacade,
                                              const int32_t mapIndex)
:
m_dataCount(ciftiFacade->getMapDataCount()),
m_paletteColorMapping(NULL),
m_labelTable(NULL),
m_rgbaValid(false)
{
    m_fastStatistics.grabNew(new FastStatistics());
    m_histogram.grabNew(new Histogram());
    m_metadata.grabNew(new GiftiMetaData());
    
    if (ciftiFacade->containsMapAttributes()) {
        ciftiFacade->getMetadataForMapOrSeriesIndex(mapIndex,
                                                    m_metadata);
    }
    m_paletteColorMapping = ciftiFacade->getPaletteColorMappingForMapOrSeriesIndex(mapIndex);
    
    m_labelTable = ciftiFacade->getLabelTableForMapOrSeriesIndex(mapIndex);
    
    /*
     * Resize RGBA.  Values are filled in updateColoring()
     */
    m_rgba.resize(m_dataCount * 4, 0.0);
}

/**
 * Destructor.
 */
CiftiMappableDataFile::MapContent::~MapContent()
{
    /**
     * Do not delete these as they point to data in CIFTI XML:
     *   m_labelTable
     *   m_paletteSettings
     */
}

/**
 * Clear the modfied status of items in the map.
 */
void
CiftiMappableDataFile::MapContent::clearModifiedStatus()
{
    if (m_labelTable != NULL) {
        m_labelTable->clearModified();
    }
    
    m_metadata->clearModified();
    
    if (m_paletteColorMapping != NULL) {
        m_paletteColorMapping->clearModified();
    }
}

/**
 * @return Modification status.
 */
bool
CiftiMappableDataFile::MapContent::isModifiedStatus()
{
    if (m_labelTable != NULL) {
        if (m_labelTable->isModified()) {
            return true;
        }
    }
    
    if (m_metadata->isModified()) {
        return true;
    }
    
    if (m_paletteColorMapping != NULL) {
        if (m_paletteColorMapping->isModified()) {
            return true;
        }
    }
    
    return false;
}

/**
 * Invalidate the coloring (usually due to palette or data changes).
 */
void
CiftiMappableDataFile::MapContent::invalidateColoring()
{
    m_rgbaValid = false;
}

/**
 * Update coloring for this map.  If the paletteFile is NOT NULL,
 * color using a palette; otherwise, color with label table.
 *
 * @param data
 *    Data contained in the map.
 * @param paletteFile
 *    File containing the palettes.
 */
void
CiftiMappableDataFile::MapContent::updateColoring(const std::vector<float>& data,
                                                  const PaletteFile* paletteFile)
{
    if (data.empty()) {
        return;
    }
    
    if (m_rgbaValid) {
        return;
    }
    
    CaretAssert(m_dataCount == static_cast<int32_t>(data.size()));
    
    if (paletteFile != NULL) {
        CaretAssert(m_paletteColorMapping);
        const AString paletteName = m_paletteColorMapping->getSelectedPaletteName();
        const Palette* palette = paletteFile->getPaletteByName(paletteName);
        if (palette != NULL) {
            m_fastStatistics->update(&data[0],
                                     data.size());
            NodeAndVoxelColoring::colorScalarsWithPalette(m_fastStatistics,
                                                          m_paletteColorMapping,
                                                          palette,
                                                          &data[0],
                                                          &data[0],
                                                          m_dataCount,
                                                          &m_rgba[0]);
        }
        else {
            std::fill(m_rgba.begin(),
                      m_rgba.end(),
                      0.0);
        }
    }
    else {
        NodeAndVoxelColoring::colorIndicesWithLabelTable(m_labelTable,
                                                         &data[0],
                                                         data.size(),
                                                         &m_rgba[0]);
    }
    
    m_rgbaValid = true;
    
    CaretLogFine("Connectivity Data Average/Min/Max: "
                 + QString::number(m_fastStatistics->getMean())
                 + " "
                 + QString::number(m_fastStatistics->getMostNegativeValue())
                 + " "
                 + QString::number(m_fastStatistics->getMostPositiveValue()));
}

/**
 * @return The units for the 'interval' between two consecutive maps.
 */
NiftiTimeUnitsEnum::Enum
CiftiMappableDataFile::getMapIntervalUnits() const
{
    return NiftiTimeUnitsEnum::NIFTI_UNITS_UNKNOWN;
}

/**
 * Get the units value for the first map and the
 * quantity of units between consecutive maps.  If the
 * units for the maps is unknown, value of one (1) are
 * returned for both output values.
 *
 * @param firstMapUnitsValueOut
 *     Output containing units value for first map.
 * @param mapIntervalStepValueOut
 *     Output containing number of units between consecutive maps.
 */
void
CiftiMappableDataFile::getMapIntervalStartAndStep(float& firstMapUnitsValueOut,
                                        float& mapIntervalStepValueOut) const
{
    firstMapUnitsValueOut   = 1.0;
    mapIntervalStepValueOut = 1.0;
}



