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
/*LICENSE_END*/

#include "TimeLine.h"
using namespace caret;
TimeLine::TimeLine()
{
    structure = StructureEnum::INVALID;
    surfaceNumberOfNodes = -1;
    colorID = -1;
    id = NULL;
    clmID = -1;
    type = NODE;
}

TimeLine::TimeLine(uint64_t nodeidIn, double *pointIn, QVector<double> &xIn, QVector<double> &yIn, ChartableBrainordinateInterface *idIn)
{
    structure = StructureEnum::INVALID;
    surfaceNumberOfNodes = -1;
    colorID = -1;
    nodeid = nodeidIn;
    for(int i=0;i<3;i++) point[i] = pointIn[i];
    x = xIn;
    y = yIn;
    id = idIn;
    clmID = -1;
}

TimeLine::~TimeLine()
{
    x.clear();
    y.clear();
}
