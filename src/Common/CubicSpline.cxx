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

#include "CubicSpline.h"
#include "CaretAssert.h"

using namespace caret;

CubicSpline::CubicSpline()
{
}

CubicSpline CubicSpline::hermite(float frac, bool lowEdge, bool highEdge)
{//these equations are derived from hermite basis functions, plug the commented m0, m1 into the hermite representation to rederive
    CaretAssert(frac > -0.01f && frac < 1.01f);//give some leeway for rounding errors
    CubicSpline ret;
    float frac2 = frac * frac;
    float frac3 = frac2 * frac;
    if (lowEdge)
    {//edge case: m0 = p[2] - p[1]
        if (highEdge)
        {//edge case: m1 = p[2] - p[1] - makes it linear interpolation - why are you doing cubic spline with only 2 points?
            ret.m_weights[0] = 0.0f;
            ret.m_weights[1] = 1.0f - frac;
            ret.m_weights[2] = frac;
            ret.m_weights[3] = 0.0f;
        } else {//m1 = (p[3] - p[1]) / 2
            ret.m_weights[0] = 0.0f;
            ret.m_weights[1] = 0.5f * frac3 - 0.5 * frac2 - frac + 1.0f;//.5t^3 - .5t^2 - t + 1
            ret.m_weights[2] = -frac3 + frac2 + frac;//-t^3 + t^2 + t
            ret.m_weights[3] = 0.5f * frac3 - frac2;//.5t^3 - t^2
        }
    } else {//m0 = (p[2] - p[0]) / 2
        if (highEdge)
        {//edge case: m1 = p[2] - p[1]
            ret.m_weights[0] = -0.5f * frac3 + frac2 - 0.5f * frac;//-.5t^3 + t^2 - .5t
            ret.m_weights[1] = frac3 - 2.0f * frac2 + 1;//t^3 - 2t^2 + 1
            ret.m_weights[2] = -0.5f * frac3 + frac2 + 0.5f * frac;//-.5t^3 + t^2 + .5t
            ret.m_weights[3] = 0.0f;
        } else {//m1 = (p[3] - p[1]) / 2 -- majority case
            ret.m_weights[0] = -0.5f * frac3 + frac2 - 0.5f * frac;//-.5t^3 + t^2 - .5t
            ret.m_weights[1] = 1.5f * frac3 - 2.5f * frac2 + 1.0f;//1.5t^3 - 2.5t^2 + 1
            ret.m_weights[2] = -1.5f * frac3 + 2.0f * frac2 + 0.5f * frac;//-1.5t^3 + 2t^2 + .5t
            ret.m_weights[3] = 0.5f * frac3 - frac2;//.5t^3 - t^2
        }
    }
    return ret;
}
