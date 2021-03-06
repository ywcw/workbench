#ifndef __SELECTION_ITEM_CHART_DATA_SERIES_H__
#define __SELECTION_ITEM_CHART_DATA_SERIES_H__

/*LICENSE_START*/
/*
 *  Copyright (C) 2014  Washington University School of Medicine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*LICENSE_END*/


#include "SelectionItem.h"



namespace caret {
    class ChartDataCartesian;
    class ChartModelDataSeries;
    
    class SelectionItemChartDataSeries : public SelectionItem {
        
    public:
        SelectionItemChartDataSeries();
        
        virtual ~SelectionItemChartDataSeries();
        
        SelectionItemChartDataSeries(const SelectionItemChartDataSeries& obj);

        SelectionItemChartDataSeries& operator=(const SelectionItemChartDataSeries& obj);
        
        virtual bool isValid() const;
        
        virtual void reset();
        
        ChartModelDataSeries* getChartModelDataSeries() const;
        
        ChartDataCartesian* getChartDataCartesian() const;
        
        int32_t getChartDataPointIndex() const;
        
        void setChart(ChartModelDataSeries* chartModelDataSeries,
                      ChartDataCartesian* chartDataCartesian,
                      const int32_t chartDataPointIndex);
        
        // ADD_NEW_METHODS_HERE

    private:
        void copyHelperSelectionItemChartDataSeries(const SelectionItemChartDataSeries& obj);

        ChartModelDataSeries* m_chartModelDataSeries;
        
        ChartDataCartesian* m_chartDataCartesian;
        
        int32_t m_chartDataPointIndex;
        
        // ADD_NEW_MEMBERS_HERE

    };
    
#ifdef __SELECTION_ITEM_CHART_DATA_SERIES_DECLARE__
    // <PLACE DECLARATIONS OF STATIC MEMBERS HERE>
#endif // __SELECTION_ITEM_CHART_DATA_SERIES_DECLARE__

} // namespace
#endif  //__SELECTION_ITEM_CHART_DATA_SERIES_H__
