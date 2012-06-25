#ifndef TIMESERIESMANAGERFORVIEWCONTROLLER_H
#define TIMESERIESMANAGERFORVIEWCONTROLLER_H
/*LICENSE_START*/
/* 
 *  Copyright 1995-2011 Washington University School of Medicine 
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
#include <stdint.h>

#include "QThread"
#include "QTimer"

class QDoubleSpinBox;

namespace caret {

    class ConnectivityTimeSeriesViewController;
/*void spinBoxChangeReceiver(QSpinBox* spinBox,
                                 const int i);
void setUpSignal(const char *object, const char *method);*/

class TimeSeriesManagerForViewController : public QObject
{
   Q_OBJECT
public:
    TimeSeriesManagerForViewController(ConnectivityTimeSeriesViewController *ctsvc);
    virtual ~TimeSeriesManagerForViewController();
    void play();    
    void stop();
    void toggleAnimation();
    void getCurrentTime();
    void setAnimationStartTime(const double &time);   
    
public slots:
    void update();
signals:
    void doubleSpinBoxValueChanged(double);
    void doubleSpinBoxValueChanged(QDoubleSpinBox*,const double);
    void start_timer(int);
    void stop_timer();
private:    
    bool m_isPlaying;
    ConnectivityTimeSeriesViewController *m_ctsvc;
    int64_t m_timeIndex;
    int64_t m_updateInterval;
    float m_timeStep;
    int64_t m_timePoints;
    QTimer *m_timer;
    QDoubleSpinBox *m_spinBox;
    double m_startTime;
    QThread *thread;
};


}
#endif // TIMESERIESMANAGERFORVIEWCONTROLLER_H
