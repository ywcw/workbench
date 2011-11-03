/*LICENSE_START*/ 
/* 
 *  Copyright 1995-2002 Washington University School of Medicine 
 * 
 *  http://brainmap.wustl.edu 
 * 
 *  This file is part of CARET. 
 * 
 *  CARET is free software; you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation; either version 2 of the License, or 
 *  (at your option) any later version. 
 * 
 *  CARET is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 *  GNU General Public License for more details. 
 * 
 *  You should have received a copy of the GNU General Public License 
 *  along with CARET; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 */ 

#include <algorithm>

#include "CaretAssert.h"
#include "CaretLogger.h"
#include "CiftiFile.h"
#include "DescriptiveStatistics.h"
#include "ElapsedTimer.h"
#include "GiftiLabelTable.h"
#include "GiftiMetaData.h"
#include "ConnectivityLoaderFile.h"
#include "PaletteColorMapping.h"
#include "SurfaceFile.h"

using namespace caret;

/**
 * Constructor.
 */
ConnectivityLoaderFile::ConnectivityLoaderFile()
: CaretMappableDataFile(DataFileTypeEnum::CONNECTIVITY_DENSE)
{
    this->ciftiDiskFile = NULL;
    this->ciftiInterface = NULL;
    this->descriptiveStatistics = NULL;
    this->paletteColorMapping = NULL;
    this->labelTable = NULL;
    this->metadata = NULL;
    this->data = NULL;
    this->dataRGBA = NULL;
    this->loaderType = LOADER_TYPE_INVALID;
}

/**
 * Destructor.
 */
ConnectivityLoaderFile::~ConnectivityLoaderFile()
{
    this->clearData();
}

/**
 * Clear all data in the file.
 */
void 
ConnectivityLoaderFile::clearData()
{
    if (this->ciftiDiskFile != NULL) {
        delete this->ciftiDiskFile;
        this->ciftiDiskFile = NULL;
    }
    if (this->descriptiveStatistics != NULL) {
        delete this->descriptiveStatistics;
        this->descriptiveStatistics = NULL;
    }
    if (this->paletteColorMapping != NULL) {
        delete this->paletteColorMapping;
        this->paletteColorMapping = NULL;
    }
    if (this->labelTable != NULL) {
        delete this->labelTable;
        this->labelTable = NULL;
    }
    if (this->metadata != NULL) {
        delete this->metadata;
        this->metadata = NULL;
    }
    this->ciftiInterface = NULL; // pointer to disk or network file so do not delete
    this->loaderType = LOADER_TYPE_INVALID;
    this->allocateData(0);
}

/**
 * Clear the contents of this file.
 */
void 
ConnectivityLoaderFile::clear()
{
    CaretMappableDataFile::clear();
    this->reset();
}

/**
 * Reset this file.
 * Clear all data and initialize needed data.
 */
void
ConnectivityLoaderFile::reset()
{
    this->clearData();
    this->descriptiveStatistics = new DescriptiveStatistics();
    
    this->paletteColorMapping = new PaletteColorMapping();
    
    this->labelTable = new GiftiLabelTable();
    
    this->metadata = new GiftiMetaData();    
}

/**
 * Is this file empty?
 *
 * @return true if file is empty, else false.
 */
bool 
ConnectivityLoaderFile::isEmpty() const
{
    return this->getFileName().isEmpty();
}

/**
 * Setup the loader.
 * @param filename
 *    Name of file from which data is obtained.
 * @param connectivityFileType
 *    Type of data.
 * @throws DataFileException
 *    If there is an error reading the file.
 */
void 
ConnectivityLoaderFile::setup(const AString& filename,
                              const DataFileTypeEnum::Enum connectivityFileType) throw (DataFileException)
{
    this->clear();
    
    switch (connectivityFileType) {
        case DataFileTypeEnum::CONNECTIVITY_DENSE:
            this->loaderType = LOADER_TYPE_DENSE;
            break;
        case DataFileTypeEnum::CONNECTIVITY_DENSE_TIME_SERIES:
            this->loaderType = LOADER_TYPE_DENSE_TIME_SERIES;
            break;
        default:
            throw DataFileException("Unsupported connectivity type " 
                                    + DataFileTypeEnum::toName(connectivityFileType));
            break;
    }
    
    try {
        if (filename.startsWith("http://")) {
            
        }
        else {
            this->ciftiDiskFile = new CiftiFile();
            this->ciftiDiskFile->openFile(filename, ON_DISK);
            this->ciftiInterface = this->ciftiDiskFile;
        }
        this->setFileName(filename);
        this->setDataFileType(connectivityFileType);
    }
    catch (CiftiFileException& e) {
        throw DataFileException(e.whatAString());
    }    
}

/**
 * Read the file.
 *
 * @param filename
 *    Name of file to read.
 *
 * @throws DataFileException
 *    If there is an error reading the file.
 */
void 
ConnectivityLoaderFile::readFile(const AString& filename) throw (DataFileException)
{
    throw DataFileException("Reading of ConnectivityLoaderFile not supported, use setup()");
}

/**
 * Write the file.
 *
 * @param filename
 *    Name of file to read.
 *
 * @throws DataFileException
 *    If there is an error writing the file.
 */
void 
ConnectivityLoaderFile::writeFile(const AString& filename) throw (DataFileException)
{
    throw DataFileException("Writing of ConnectivityLoaderFile not supported.");
}

/**
 * Get information about this file's contents.
 * @return
 *    Information about the file's contents.
 */
AString 
ConnectivityLoaderFile::toString() const
{
    return "ConnectivityLoaderFile";
}

StructureEnum::Enum 
ConnectivityLoaderFile::getStructure() const
{
    return StructureEnum::ALL;
}

/**
 * Set the structure.
 * @param structure 
 *    New value for file's structure.
 */
void 
ConnectivityLoaderFile::setStructure(const StructureEnum::Enum structure)
{
    /* do nothing */
}

/**
 * @return Get access to the file's metadata.
 */
GiftiMetaData* 
ConnectivityLoaderFile::getFileMetaData()
{
    return this->metadata;
}

/**
 * @return Get access to unmodifiable file's metadata.
 */
const GiftiMetaData* 
ConnectivityLoaderFile::getFileMetaData() const
{
    return this->metadata;
}

/**
 * @return The palette color mapping for a data column.
 */
PaletteColorMapping* 
ConnectivityLoaderFile::getPaletteColorMapping(const int32_t columnIndex)
{
    return this->paletteColorMapping;
}

/**
 * @return Is the data mappable to a surface?
 */
bool 
ConnectivityLoaderFile::isSurfaceMappable() const
{
    return true;
}

/**
 * @return Is the data mappable to a volume?
 */
bool 
ConnectivityLoaderFile::isVolumeMappable() const
{
    return true;
}

/**
 * @return The number of maps in the file.  
 * Note: Caret5 used the term 'columns'.
 */
int32_t 
ConnectivityLoaderFile::getNumberOfMaps() const
{
    int32_t numMaps = 0;
    
    if (this->ciftiInterface != NULL) {
        switch (this->loaderType) {
            case LOADER_TYPE_INVALID:
                break;
            case LOADER_TYPE_DENSE:
                numMaps = 1;
                break;
            case LOADER_TYPE_DENSE_TIME_SERIES:
                numMaps = this->ciftiInterface->getNumberOfColumns();
                break;
        }
    }
    
    return numMaps;
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
ConnectivityLoaderFile::getMapName(const int32_t mapIndex) const
{
    return "Map " + AString::number(mapIndex + 1);
}

/**
 * Find the index of the map that uses the given name.
 * 
 * @param mapName
 *    Name of the desired map.
 * @return
 *    Index of the map using the given name.  If there is more
 *    than one map with the given name, this method is likely
 *    to return the index of the first map with the name.
 */
int32_t 
ConnectivityLoaderFile::getMapIndexFromName(const AString& mapName)
{
    return 0;
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
ConnectivityLoaderFile::setMapName(const int32_t mapIndex,
                        const AString& mapName)
{
    
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
ConnectivityLoaderFile::getMapMetaData(const int32_t mapIndex) const
{
    return this->metadata;
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
ConnectivityLoaderFile::getMapMetaData(const int32_t mapIndex)
{
    return this->metadata;
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
ConnectivityLoaderFile::getMapStatistics(const int32_t mapIndex)
{
    this->descriptiveStatistics->update(this->data, 
                                        this->numberOfDataElements);
    return this->descriptiveStatistics;
}

/**
 * @return Is the data in the file mapped to colors using
 * a palette.
 */
bool 
ConnectivityLoaderFile::isMappedWithPalette() const
{
    return true;
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
ConnectivityLoaderFile::getMapPaletteColorMapping(const int32_t mapIndex)
{
    return this->paletteColorMapping;    
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
ConnectivityLoaderFile::getMapPaletteColorMapping(const int32_t mapIndex) const
{
    return this->paletteColorMapping;    
}

/**
 * @return Is the data in the file mapped to colors using
 * a label table.
 */
bool 
ConnectivityLoaderFile::isMappedWithLabelTable() const
{
    return false;
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
ConnectivityLoaderFile::getMapLabelTable(const int32_t /*mapIndex*/)
{
    /*
     * Use file's label table since GIFTI uses one
     * label table for all data arrays.
     */
    return this->labelTable;
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
ConnectivityLoaderFile::getMapLabelTable(const int32_t /*mapIndex*/) const
{
    /*
     * Use file's label table since GIFTI uses one
     * label table for all data arrays.
     */
    return this->labelTable;
}

/**
 * @return Is this loading dense connectivity data?
 */
bool 
ConnectivityLoaderFile::isDense() const
{
    return (this->loaderType == LOADER_TYPE_DENSE);
}

/**
 * @return Is this loading dense time series connectivity data?
 */
bool 
ConnectivityLoaderFile::isDenseTimeSeries() const
{
    return (this->loaderType == LOADER_TYPE_DENSE_TIME_SERIES);
}

/**
 * @return Name describing loader content.
 */
AString 
ConnectivityLoaderFile::getCiftiTypeName() const
{
    switch (this->loaderType) {
        case LOADER_TYPE_INVALID:
            break;
        case LOADER_TYPE_DENSE:
            return "Dense";
            break;
        case LOADER_TYPE_DENSE_TIME_SERIES:
            return "Dense Time";
            break;
    }
    return "";
}

/**
 * Allocate data.
 * @param numberOfDataElements
 *    Number of elements in the data.
 */
void 
ConnectivityLoaderFile::allocateData(const int32_t numberOfDataElements)
{
    if (numberOfDataElements != this->numberOfDataElements) {
        if (data != NULL) {
            delete data;
            this->data = NULL;
        }
        if (dataRGBA != NULL) {
            delete this->dataRGBA;
            this->dataRGBA = NULL;
        }
        
        this->numberOfDataElements = numberOfDataElements;
        
        if (numberOfDataElements > 0) {
            this->data = new float[this->numberOfDataElements];
            this->dataRGBA = new float[this->numberOfDataElements * 4];
        }
    }    
}

/**
 * Zero out the data such as when loading data fails.
 */
void 
ConnectivityLoaderFile::zeroizeData()
{
    std::fill(this->data, 
              this->data + this->numberOfDataElements,
              0.0);    
}

/**
 * Load connectivity data for the surface's node.
 * @param surfaceFile
 *    Surface file used for structure.
 * @param nodeIndex
 *    Index of node number.
 */
void 
ConnectivityLoaderFile::loadDataForSurfaceNode(const StructureEnum::Enum structure,
                                               const int32_t nodeIndex) throw (DataFileException)
{
    if (this->ciftiInterface == NULL) {
        throw DataFileException("Connectivity Loader has not been initialized");
    }
    
    std::cout << "Connectivity Load Surface: "
    << StructureEnum::toGuiName(structure)
    << " Node: "
    << nodeIndex
    << std::endl;
    
    try {
        switch (this->loaderType) {
            case LOADER_TYPE_INVALID:
                break;
            case LOADER_TYPE_DENSE:
            {
                const int32_t num = this->ciftiInterface->getNumberOfRows();
                this->allocateData(num);
                
                if (this->ciftiInterface->getRowFromNode(this->data, 
                                                         nodeIndex,
                                                         structure)) {
                    std::cout << "Read row for node " << nodeIndex << std::endl;
                }
                else {
                    std::cout << "FAILED to read row for node " << nodeIndex << std::endl;
                    this->zeroizeData();
                }
            }
                break;
            case LOADER_TYPE_DENSE_TIME_SERIES:
                break;
        }
    }
    catch (CiftiFileException& e) {
        throw DataFileException(e.whatAString());
    }
}

/**
 * Load data for a voxel at the given coordinate.
 * @param xyz
 *    Coordinate of voxel.
 */
void 
ConnectivityLoaderFile::loadDataForVoxelAtCoordinate(const float xyz[3]) throw (DataFileException)
{
    if (this->ciftiInterface == NULL) {
        throw DataFileException("Connectivity Loader has not been initialized");
    }
    
    std::cout << "Connectivity Load Voxel: "
    << AString::fromNumbers(xyz, 3, ", ")
    << std::endl;
    
    try {
        switch (this->loaderType) {
            case LOADER_TYPE_INVALID:
                break;
            case LOADER_TYPE_DENSE:
            {
                const int32_t num = this->ciftiInterface->getNumberOfRows();
                this->allocateData(num);
                
                //                if (this->ciftiInterface->getRowFromVoxel(this->data, 
                //                                                        nodeIndex,
                //                                                        surfaceFile->getStructure())) {
                //                    std::cout << "Read row for voxel " << AString::fromNumber(xyz, 3, ",") << std::endl;
                //                }
                //                else {
                //                    std::cout << "FAILED to read row for voxel " << AString::fromNumber(xyz, 3, ",") << std::endl;
                //                }
            }
                break;
            case LOADER_TYPE_DENSE_TIME_SERIES:
                break;
        }
    }
    catch (CiftiFileException& e) {
        throw DataFileException(e.whatAString());
    }
}

/**
 * @return Number of elements in the data that was loaded.
 */
int32_t 
ConnectivityLoaderFile::getNumberOfDataElements() const
{
    return this->numberOfDataElements;
}

/**
 * @return Pointer to data that was loaded which contains
 * "getNumberOfDataElements()" elements.
 */
float* 
ConnectivityLoaderFile::getData()
{
    return this->data;
}

/**
 * @return Pointer to RGBA coloring for data that was loaded which contains
 * "getNumberOfDataElements() * 4" elements.
 */
float* 
ConnectivityLoaderFile::getDataRGBA()
{
    return this->dataRGBA;
}

/**
 * Get the node coloring for the surface.
 * @param surface
 *    Surface whose nodes are colored.
 * @param nodeRGBA
 *    Filled with RGBA coloring for the surface's nodes. 
 *    Contains numberOfNodes * 4 elements.
 * @param numberOfNodes
 *    Number of nodes in the surface.
 * @return 
 *    True if coloring is valid, else false.
 */
bool 
ConnectivityLoaderFile::getSurfaceNodeColoring(const StructureEnum::Enum structure,
                                               float* nodeRGBA,
                                               const int32_t numberOfNodes)
{
    if (this->numberOfDataElements <= 0) {
        return false;
    }
    
    bool useColumnsFlag = false;
    switch (this->loaderType) {
        case LOADER_TYPE_INVALID:
            break;
        case LOADER_TYPE_DENSE:
            useColumnsFlag = true;
            break;
        case LOADER_TYPE_DENSE_TIME_SERIES:
            break;
    }
    
    if (useColumnsFlag) {
        std::vector<CiftiSurfaceMap> nodeMap;
        this->ciftiInterface->getSurfaceMapForColumns(nodeMap, structure);
        
        std::fill(nodeRGBA, (nodeRGBA + (numberOfNodes * 4)), 0.0);
        const int64_t numNodeMaps = static_cast<int32_t>(nodeMap.size());
        for (int i = 0; i < numNodeMaps; i++) {
            const int64_t node4 = nodeMap[i].m_surfaceNode * 4;
            const int64_t cifti4 = nodeMap[i].m_ciftiIndex * 4;
            CaretAssertArrayIndex(nodeRGBA, (numberOfNodes * 4), node4);
            CaretAssertArrayIndex(this->dataRGBA, (this->numberOfDataElements * 4), cifti4);
            nodeRGBA[node4]   = this->dataRGBA[cifti4];
            nodeRGBA[node4+1] = this->dataRGBA[cifti4+1];
            nodeRGBA[node4+2] = this->dataRGBA[cifti4+2];
            nodeRGBA[node4+3] = this->dataRGBA[cifti4+3];
        }
        return true;
    }
    
    return false;
}


