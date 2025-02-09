/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * <OWNER> = Apple Inc.
 * <ORGANIZATION> = Apple Inc.
 * <YEAR> = 2017
 *
 * Copyright (c) 2017, Apple Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the <ORGANIZATION> nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*!
 *************************************************************************************
 * \file FrameScaleGaussian.cpp
 *
 * \brief
 *    Gaussian Filter Interpolation (JVT-R006/JVT-Q007)
 *
 * \author
 *     - Alexis Michael Tourapis         <atourapis@apple.com>
 *
 *************************************************************************************
 */

//-----------------------------------------------------------------------------
// Include headers
//-----------------------------------------------------------------------------

#include "Global.H"
#include "FrameScaleGaussian.H"
#include <string.h>

//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------

namespace hdrtoolslib {

//-----------------------------------------------------------------------------
// Constructor/destructor
//-----------------------------------------------------------------------------

FrameScaleGaussian::FrameScaleGaussian(int iWidth, int iHeight, int oWidth, int oHeight, int lobes, ChromaLocation chromaLocationType) {
  
  m_lobes = lobes;
  m_lobesX = lobes;
  m_lobesY = lobes;
  
  m_pi = (double) HDR_PI  / (double) m_lobes;
  m_piSq = (double) HDR_PI * m_pi;
  
  /* if resampling is actually downsampling we have to EXTEND the length of the
   original filter; the ratio is calculated below */
  m_factorY = (double)(iHeight) / (double) (oHeight);
  m_factorX = (double)(iWidth ) / (double) (oWidth );
  
  //  m_offsetX = m_factorX > 1.0 ? 1 / m_factorX : 0.0;
  //  m_offsetY = m_factorY > 1.0 ? 1 / m_factorY : 0.0;
  m_offsetX = 0.0;
  m_offsetY = 0.0;

  m_filterTapsX = (m_factorX == 1.0 ) ? 1 : (m_factorX > 1.0 ) ? (int) ceil (m_factorX * 2 * m_lobesX) :  2 * m_lobesX;
  m_filterTapsY = (m_factorY == 1.0 ) ? 1 : (m_factorY > 1.0 ) ? (int) ceil (m_factorY * 2 * m_lobesY) :  2 * m_lobesY;

  // Allocate filter memory
  m_filterOffsetsX.resize(m_filterTapsX);
  m_filterOffsetsY.resize(m_filterTapsY);
  
  // We basically allocate filter coefficients for all target positions.
  // This is done once and saves us time from deriving the proper filter for each position.
  m_filterCoeffsX.resize(oWidth * m_filterTapsX);
  m_filterCoeffsY.resize(oHeight * m_filterTapsY);
  
  // Set integer filter precision in bits
  m_filterPrecision = 14; 

  // Allocate integer versions
  m_filterIntCoeffsX.resize(oWidth * m_filterTapsX);
  m_filterIntCoeffsY.resize(oHeight * m_filterTapsY);
  
  // Initialize the filter boundaries
  SetFilterLimits((int *) &m_filterOffsetsX[0], m_filterTapsX, iWidth,  oWidth,  m_factorX, &m_iMinX, &m_iMaxX, &m_oMinX, &m_oMaxX);
  SetFilterLimits((int *) &m_filterOffsetsY[0], m_filterTapsY, iHeight, oHeight, m_factorY, &m_iMinY, &m_iMaxY, &m_oMinY, &m_oMaxY);
  
  // Finally prepare the filter coefficients for horizontal and vertical filtering
  // Horizontal
  //PrepareFilterCoefficients((double *) &m_filterCoeffsX[0], (int *) &m_filterOffsetsX[0], m_factorX, m_filterTapsX, m_offsetX, oWidth, m_lobesX);
  
  PrepareFilterCoefficients((double *) &m_filterCoeffsX[0], (int *) &m_filterIntCoeffsX[0], (int *) &m_filterOffsetsX[0], m_factorX, m_filterTapsX, m_offsetX, oWidth, m_lobesX, m_filterPrecision);
  
  // Vertical
  //PrepareFilterCoefficients((double *) &m_filterCoeffsY[0], (int *) &m_filterOffsetsY[0], m_factorY, m_filterTapsY, m_offsetY, oHeight, m_lobesY);
  PrepareFilterCoefficients((double *) &m_filterCoeffsY[0], (int *) &m_filterIntCoeffsY[0], (int *) &m_filterOffsetsY[0], m_factorY, m_filterTapsY, m_offsetY, oHeight, m_lobesY, m_filterPrecision);

}

//FrameScaleGaussian::~FrameScaleGaussian() {
//}

//-----------------------------------------------------------------------------
// Private Methods
//-----------------------------------------------------------------------------

double FrameScaleGaussian::FilterTap( double dist, double piScaled, double factor, int lobes ) {
  dist = dAbs(dist);
  double SD = factor * 0.50;
  double L = lobes * SD;
  
  if ( dist > L )
    return 0.0;
  else {
    double xL = dist / SD;
    return exp( - xL * xL /2 );
  }
}

//-----------------------------------------------------------------------------
// Public methods
//-----------------------------------------------------------------------------

void FrameScaleGaussian::process ( Frame* out, const Frame *inp)
{
  if (( out->m_isFloat != inp->m_isFloat ) || (( inp->m_isFloat == 0 ) && ( out->m_bitDepth != inp->m_bitDepth ))) {
    fprintf(stderr, "Error: trying to copy frames of different data types. \n");
    exit(EXIT_FAILURE);
  }
    
  int c;
  
  out->m_frameNo = inp->m_frameNo;
  out->m_isAvailable = TRUE;
  
  for (c = Y_COMP; c <= V_COMP; c++) {
    out->m_minPelValue[c]  = inp->m_minPelValue[c];
    out->m_midPelValue[c]  = inp->m_midPelValue[c];
    out->m_maxPelValue[c]  = inp->m_maxPelValue[c];
  }
  if (out->m_isFloat == TRUE) {    // floating point data
    for (c = Y_COMP; c <= V_COMP; c++) {
      filter( inp->m_floatComp[c], out->m_floatComp[c], inp->m_width[c], inp->m_height[c], out->m_width[c], out->m_height[c], (double) out->m_minPelValue[c], (double) out->m_maxPelValue[c] );
  //      filter( inp->m_floatComp[c], out->m_floatComp[c], inp->m_width[c], inp->m_height[c], out->m_width[c], out->m_height[c], -10000000.0, 10000000.0);
  }
  }
  else if (out->m_bitDepth == 8) {   // 8 bit data
    for (c = Y_COMP; c <= V_COMP; c++) {
      filter( inp->m_comp[c], out->m_comp[c], inp->m_width[c], inp->m_height[c], out->m_width[c], out->m_height[c], out->m_minPelValue[c], out->m_maxPelValue[c], 1 );
    }
  }
  else { // 16 bit data
    for (c = Y_COMP; c <= V_COMP; c++) {
      filter( inp->m_ui16Comp[c], out->m_ui16Comp[c], inp->m_width[c], inp->m_height[c], out->m_width[c], out->m_height[c], out->m_minPelValue[c], out->m_maxPelValue[c], 1 );
    }
  }
}
} // namespace hdrtoolslib


//-----------------------------------------------------------------------------
// End of file
//-----------------------------------------------------------------------------
