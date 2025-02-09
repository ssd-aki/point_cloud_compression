/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * <OWNER> = Apple Inc.
 * <ORGANIZATION> = Apple Inc.
 * <YEAR> = 2015
 *
 * Copyright (c) 2015, Apple Inc.
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
 * \file FrameScale.cpp
 *
 * \brief
 *    Base Class for Frame Scaling
 *
 * \author
 *     - Alexis Michael Tourapis         <atourapis@apple.com>
 *
 *************************************************************************************
 */

//-----------------------------------------------------------------------------
// Include headers
//-----------------------------------------------------------------------------

#include <math.h>
#include "Global.H"
#include "FrameScale.H"
#include "FrameScaleCopy.H"
#include "FrameScaleNN.H"
#include "FrameScaleNull.H"
#include "FrameScaleHalf.H"
#include "FrameScaleBilinear.H"
#include "FrameScaleBiCubic.H"
#include "FrameScaleLanczos.H"
#include "FrameScaleLanczosInt.H"
#include "FrameScaleHanning.H"
#include "FrameScaleHamming.H"
#include "FrameScaleSineWindow.H"
#include "FrameScaleGaussian.H"
#include "FrameScaleSHVC.H"
#include "FrameScaleLanczosCTC.H"

namespace hdrtoolslib {

//-----------------------------------------------------------------------------
// Private methods
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Protected methods
//-----------------------------------------------------------------------------
void FrameScale::PrepareFilterCoefficients(double *factorCoeffs, int *filterOffsets, double factor, int filterTaps, double offset, int oSize)
{
  int x;
  int tapIndex, index;
  double coeffSum, dist;
  double off, posOrig;

  for ( x = 0, index = 0; x < oSize; x++, index += filterTaps ) {
    posOrig = offset + (double) x * factor;
    off     = posOrig - floor(posOrig);

    coeffSum = 0.0;

    for ( tapIndex = index; tapIndex < index + filterTaps; tapIndex++ ) {
      if ( factor <= 1.0 ) {
        dist = dAbs(filterOffsets[tapIndex - index] - off);
      }
      else {
        dist = dAbs(-filterOffsets[tapIndex - index] + off);
      }

      factorCoeffs[tapIndex] = FilterTap( dist );
      coeffSum += factorCoeffs[tapIndex];
    }

    for ( tapIndex = index; tapIndex < index + filterTaps; tapIndex++ ) {
      factorCoeffs[ tapIndex ] /= coeffSum;
    }
  }
}

void FrameScale::PrepareFilterCoefficients(double *factorCoeffs, int *filterOffsets, double factor, int filterTaps, double offset, int oSize, int lobes)
{
  int x;
  int tapIndex, index;
  double coeffSum, dist;
  double off, posOrig;
  
  double pi = (double) HDR_PI  / (double) lobes;

  for ( x = 0, index = 0; x < oSize; x++, index += filterTaps ) {
    posOrig = offset + (double) x * factor;
    off     = posOrig - (double) floor(posOrig);

    coeffSum = 0.0;
    for ( tapIndex = index; tapIndex < index + filterTaps; tapIndex++ ) {
      if ( factor <= 1.0 ) {
        dist = dAbs(filterOffsets[tapIndex - index] - off);
      }
      else {
        dist = dAbs(-filterOffsets[tapIndex - index] + off);
      }
      
      factorCoeffs[tapIndex] = FilterTap( dist,  pi, factor > 1.0 ? factor : 1.0, lobes);
      coeffSum = coeffSum + factorCoeffs[tapIndex];
    }
    
    for ( tapIndex = index; tapIndex < index + filterTaps; tapIndex++ ) {
      factorCoeffs[ tapIndex ] = factorCoeffs[ tapIndex ] / coeffSum;
    }
  }
}

void FrameScale::PrepareFilterCoefficients(double *factorCoeffs, int *iFactorCoeffs, int *filterOffsets, double factor, int filterTaps, double offset, int oSize, int lobes, int precision)
{
  int x;
  int tapIndex, index;
  double coeffSum, dist;
  double off, posOrig;
  
  int scale = 1 << precision;
  
  double pi = (double) HDR_PI  / (double) lobes;

  for ( x = 0, index = 0; x < oSize; x++, index += filterTaps ) {
    posOrig = offset + (double) x * factor;
    off     = posOrig - floor(posOrig);

    coeffSum = 0.0;
    for ( tapIndex = index; tapIndex < index + filterTaps; tapIndex++ ) {
      if ( factor <= 1.0 ) {
        dist = dAbs(filterOffsets[tapIndex - index] - off);
      }
      else {
        dist = dAbs(-filterOffsets[tapIndex - index] + off);
      }
      
      factorCoeffs[tapIndex] = FilterTap( dist,  pi, factor > 1.0 ? factor : 1.0, lobes);
      coeffSum += factorCoeffs[tapIndex];
    }
    for ( tapIndex = index; tapIndex < index + filterTaps; tapIndex++ ) {
      factorCoeffs[ tapIndex ] /= coeffSum;
    }
    
    int coeffSum = 0;
    for ( tapIndex = index; tapIndex < index + filterTaps; tapIndex++ ) {
      iFactorCoeffs[ tapIndex ] = double2int( factorCoeffs[ tapIndex ] * (double) scale );
      coeffSum += iFactorCoeffs[ tapIndex ];
    }

    iFactorCoeffs[ index + ((filterTaps + 1 ) >> 1) ] +=  (scale - coeffSum);

    
//    if (filterTaps > 1) {
//      printf("filter %d %d\n", x, filterTaps);
//      
//      for ( tapIndex = index; tapIndex < index + filterTaps; tapIndex++ ) {
//        printf("%d %d %10.7f\n", tapIndex, iFactorCoeffs[ tapIndex ], factorCoeffs[ tapIndex ] );
//      }
//      printf("\n");
//    }
     
  }
}

void FrameScale::SetFilterLimits(int *filterOffsets, int filterTaps, int iDimension, int oDimension, double factor, int *iMin, int *iMax, int *oMin, int *oMax)
{
  int ix, iy;

  for ( ix = 0, iy = -((filterTaps - 1) >> 1); ix < filterTaps; ix++, iy++ ) {
    filterOffsets[ix] = iy;
  }
  *iMax = iDimension - 1;
  if (factor == 1)
    *oMin = 0;
  else {
    *iMin = ((filterTaps + 1) >> 1) + 2;
    if (factor <= 1)
      *oMin = (int) ((filterTaps + 1)/ factor + 1);
    else
      *oMin = (int) (filterTaps + 2);
  }
  *oMax = oDimension - *oMin; // maybe we should be more conservative here...
}


//-----------------------------------------------------------------------------
// Public methods
//-----------------------------------------------------------------------------
FrameScale *FrameScale::create(int iWidth, int iHeight, int oWidth, int oHeight, FrameScaleParams *params, int filter, ChromaLocation chromaLocationType, int useMinMax) {
  FrameScale *result = NULL;
  
  if ((iWidth == oWidth && iHeight == oHeight) || params == NULL) { // Do nothing
    result = new FrameScaleNull();
  }
  else {
    switch (params->m_mode) {
      case SC_NULL:
        result = new FrameScaleCopy(iWidth, iHeight, oWidth, oHeight);
        break;
      case SC_NN:
        result = new FrameScaleNN(iWidth, iHeight, oWidth, oHeight);
        break;
      case SC_LINEAR:
        if (iWidth == 2 * oWidth && iHeight == 2 * oHeight) {
          result = new FrameScaleHalf(iWidth, iHeight, filter, chromaLocationType, useMinMax);
        }
        else {
        result = new FrameScaleBilinear(iWidth, iHeight, oWidth, oHeight);
        }
        break;
      case SC_BILINEAR:
        result = new FrameScaleBilinear(iWidth, iHeight, oWidth, oHeight);
        break;
      case SC_LANCZOS:
        result = new FrameScaleLanczos(iWidth, iHeight, oWidth, oHeight, params->m_lanczosLobes, chromaLocationType);
        break;
      case SC_HANNING:
        result = new FrameScaleHanning(iWidth, iHeight, oWidth, oHeight, params->m_lanczosLobes, chromaLocationType);
        break;
      case SC_HAMMING:
        result = new FrameScaleHamming(iWidth, iHeight, oWidth, oHeight, params->m_lanczosLobes, chromaLocationType);
        break;
      case SC_SINWINDOW:
        result = new FrameScaleSineWindow(iWidth, iHeight, oWidth, oHeight, params->m_lanczosLobes, chromaLocationType);
        break;
      case SC_GAUSSIAN:
        result = new FrameScaleGaussian(iWidth, iHeight, oWidth, oHeight, params->m_lanczosLobes, chromaLocationType);
        break;
      case SC_BICUBIC:
        result = new FrameScaleBiCubic(iWidth, iHeight, oWidth, oHeight);
        break;
      case SC_SHVC:
        result = new FrameScaleSHVC(iWidth, iHeight, oWidth, oHeight);
        break;
      case SC_LANCZOS_INT:
        result = new FrameScaleLanczosInt(iWidth, iHeight, oWidth, oHeight, params->m_lanczosLobes, chromaLocationType, 0, 14);
        break;        
      case SC_LANCZOS_CTC:
        result = new FrameScaleLanczosCTC(iWidth, iHeight, oWidth, oHeight);        
        break;        
      default:
        fprintf(stderr, "\nUnsupported Scaling mode %d\n", params->m_mode);
        exit(EXIT_FAILURE);
    }
  }
  
  return result;
}

imgpel FrameScale::filter( imgpel *input, int *filterX, int *filterY, int filterPrecision, int posX, int posY, int iSizeX, int vMin, int vMax)
{
  int64 result = (int64) 0;
  int i, j, yPos;
  int *pFilterX = NULL, *pFilterY = filterY;
  
  // the process below generates the filtered sample in place and without the use of any additional memory.
  for ( j = 0; j < m_filterTapsY; j++, pFilterY++ ) {
    pFilterX = filterX;

    if(*pFilterY == 0) continue;
    yPos = iSizeX * iClip(posY + m_filterOffsetsY[j], 0, m_iMaxY);
    for ( i = 0; i < m_filterTapsX; i++ )
    {
      result += (int64) (*pFilterY) * (int64) (*pFilterX) * (int64) input[ yPos + iClip(posX + m_filterOffsetsX[i], 0, m_iMaxX)];
      pFilterX++;
    }
  }
  
  //printf("(%d %d) result %lld", posX, posY, result);
  result += ((int64) 1 << (2 * filterPrecision - 1));
  //printf(" %lld", result);
  result >>= ( 2 * filterPrecision );
  //printf(" %lld\n", result);
  return (imgpel) (iClip((int) result, vMin, vMax));
}

imgpel FrameScale::filter( imgpel *input, double *filterX, double *filterY, int posX, int posY, int iSizeX, int vMin, int vMax)
{
  double result = 0.0;
  double value = 0.0;
  int i, j, yPos;
  double *pFilterX = NULL, *pFilterY = filterY;
  
  // the process below generates the filtered sample in place and without the use of any
  // additional memory.
  for ( j = 0; j < m_filterTapsY; j++, pFilterY++ ) {
    pFilterX = filterX;
    yPos = iSizeX * iClip(posY + m_filterOffsetsY[j], 0, m_iMaxY);
    for ( i = 0; i < m_filterTapsX; i++ ) {
      double value = (double)(*pFilterY) * (double) (*pFilterX);
      value = value * (double) input[ yPos + iClip(posX + m_filterOffsetsX[i], 0, m_iMaxX)];
      result = (double) result + (double) value;
      pFilterX++;
    }
    //tempIn += iSizeX - m_filterTapsX;
  }

  return (imgpel) (iClip((int) (result + 0.5), vMin, vMax));
}

imgpel FrameScale::filter( imgpel *input, int *filterX, int *filterY, int filterTapsX, int filterTapsY, int *filterOffsetsX, int *filterOffsetsY, int posX, int posY, int iSizeX, int vMin, int vMax)
{
  int64 result = 0;
  int i, j, yPos;
  int *pFilterX = NULL, *pFilterY = filterY;
  int *pOffsetX = NULL, *pOffsetY = filterOffsetsY;
  
  // the process below generates the filtered sample in place and without the use of any
  // additional memory.
  for ( j = 0; j < filterTapsY; j++ ) {
    pFilterX = filterX;
    pOffsetX = filterOffsetsX;
    yPos = iSizeX * iClip(posY + *pOffsetY, 0, m_iMaxY);
    for ( i = 0; i < filterTapsX; i++ ) {
      result += (*pFilterY) * (*pFilterX) * (int64) input[ yPos + iClip(posX + *pOffsetX, 0, m_iMaxX)];
      pFilterX++;
      pOffsetX++;
    }
    pOffsetY++;
    pFilterY++;
  }
  
  return (imgpel) (iClip((int) i64ShiftRightRound(result, 28), vMin, vMax));
}


uint16 FrameScale::filter( uint16 *input, int *filterX, int *filterY, int filterPrecision, int posX, int posY, int iSizeX, int vMin, int vMax)
{
  int64 result = (int64) 0;
  int i, j, yPos;
  int *pFilterX = NULL, *pFilterY = filterY;
  
  // the process below generates the filtered sample in place and without the use of any
  // additional memory.
  for ( j = 0; j < m_filterTapsY; j++, pFilterY++ ) {
    pFilterX = filterX;
    yPos = iSizeX * iClip(posY + m_filterOffsetsY[j], 0, m_iMaxY);
    for ( i = 0; i < m_filterTapsX; i++ ) {

      result += (int64) (*pFilterY) * (int64) (*pFilterX) * (int64) input[ yPos + iClip(posX + m_filterOffsetsX[i], 0, m_iMaxX)];

      pFilterX++;
    }
  }
  
  //printf("(%d %d) result %lld", posX, posY, result);
  result += ((int64) 1 << (2 * filterPrecision - 1));
  //printf(" %lld", result);
  result >>= ( 2 * filterPrecision );
  //printf(" %lld\n", result);
  return (uint16) (iClip((int) result, vMin, vMax));
}

uint16 FrameScale::filter( uint16 *input, double *filterX, int *offsetX, int tapsX, int maxX, double *filterY, int *offsetY, int tapsY, int maxY, int posX, int posY, int iSizeX, int vMin, int vMax, int scale)
{
  int64 result = 0.0;
  int i, j, yPos;
  double *pFilterX = NULL, *pFilterY = filterY;

  // the process below generates the filtered sample in place and without the use of any
  // additional memory.
  for ( j = 0; j < tapsY; j++, pFilterY++ ) {
    pFilterX = filterX;
    yPos = iSizeX * iClip(posY + offsetY[j], 0, maxY);
    for ( i = 0; i < tapsX; i++ ) {
      result += (*pFilterY) * (*pFilterX++) * input[ yPos + iClip(posX + offsetX[i], 0, maxX)];
    }
  }
  
  return (uint16) (iClip((int) (result + scale / 2) / scale, vMin, vMax));
}


uint16 FrameScale::filter( uint16 *input, double *filterX, double *filterY, int posX, int posY, int iSizeX, int vMin, int vMax)
{
  double result = 0.0;
  int i, j, yPos;
  double *pFilterX = NULL, *pFilterY = filterY;

  //uint16 *tempIn = input + iSizeX * (posY - ((m_filterTapsY - 1 )>> 1)) + posX - ((m_filterTapsX - 1)>> 1);
  
  // the process below generates the filtered sample in place and without the use of any
  // additional memory.
  for ( j = 0; j < m_filterTapsY; j++, pFilterY++ ) {
    pFilterX = filterX;
    yPos = iSizeX * iClip(posY + m_filterOffsetsY[j], 0, m_iMaxY);
    for ( i = 0; i < m_filterTapsX; i++ ) {
      result += (*pFilterY) * (*pFilterX++) * (double) input[ yPos + iClip(posX + m_filterOffsetsX[i], 0, m_iMaxX)];
    }
  //  tempIn += iSizeX - m_filterTapsX;
  }
  
  return (uint16) (iClip((int) (result + 0.5), vMin, vMax));
}

uint16 FrameScale::filter( uint16 *input, int *filterX, int *filterY, int filterTapsX, int filterTapsY, int *filterOffsetsX, int *filterOffsetsY, int posX, int posY, int iSizeX, int vMin, int vMax)
{
  int64 result = 0;
  int i, j, yPos;
  int *pFilterX = NULL, *pFilterY = filterY;
  int *pOffsetX = NULL, *pOffsetY = filterOffsetsY;
  
  // the process below generates the filtered sample in place and without the use of any
  // additional memory.
  for ( j = 0; j < filterTapsY; j++ ) {
    pFilterX = filterX;
    pOffsetX = filterOffsetsX;
    yPos = iSizeX * iClip(posY + *pOffsetY, 0, m_iMaxY);

   // printf("(%d %d) [%d]", posY, posX, iClip(posY + *pOffsetY, 0, m_iMaxY));
    
    for ( i = 0; i < filterTapsX; i++ ) {
      result += (*pFilterY) * (*pFilterX) * (int64) input[ yPos + iClip(posX + *pOffsetX, 0, m_iMaxX)];
      pFilterX++;
      pOffsetX++;
    //  printf(" %d", iClip(posX + *pOffsetX, 0, m_iMaxX));
    }
    //printf("\n");
    pOffsetY++;
    pFilterY++;
  }
  //printf("\n");
  
  return (uint16) (iClip((int) i64ShiftRightRound(result, 28), vMin, vMax));
}

float FrameScale::filter( float *input, double *filterX, double *filterY, int posX, int posY, int iSizeX, double vMin, double vMax)
{
  double result = 0.0;
  int i, j, yPos;
  double *pFilterX = NULL, *pFilterY = filterY;

  // the process below generates the filtered sample in place and without the use of any
  // additional memory.
  
  /* interpolate point - we assume symmetric filter in each direction */
  for ( j = 0; j < m_filterTapsY; j++, pFilterY++ ) {
    pFilterX = filterX;
    yPos = iSizeX * iClip(posY + m_filterOffsetsY[j], 0, m_iMaxY);
    for ( i = 0; i < m_filterTapsX; i++ ) {
      result = result + (double)(*pFilterY) * (double)(*pFilterX++) * (double) input[ yPos + iClip(posX + m_filterOffsetsX[i], 0, m_iMaxX)];
    }
  }
  
  return (float) result; //(dClip(result, vMin, vMax));
}

void FrameScale::filter( imgpel *input, imgpel *output, int iSizeX, int iSizeY, int oSizeX, int oSizeY, int vMin, int vMax, int iFilter )
{
  /* we assume the input and output images are stored in raster scan mode */
  int x, y, filterX, filterY = 0;
  
  int origY, origX;
  imgpel *pOutput = output;
  
  m_iMaxX = iSizeX - 1;
  m_iMaxY = iSizeY - 1;

  if (iFilter == 0) {
    for ( y = 0; y < oSizeY; y++) {
      // project y to old coordinates
      origY = (int)(y * m_factorY);
      filterX = 0;
      for ( x = 0; x < oSizeX; x++) {
        // project x to old coordinates
        origX = (int)(x * m_factorX);
        pOutput = output + oSizeX * y + x;
        *pOutput = filter( input, &m_filterCoeffsX[filterX], &m_filterCoeffsY[filterY], origX, origY, iSizeX,  vMin, vMax);
        filterX += m_filterTapsX;
      }
      filterY += m_filterTapsY;
    }
  }
  else {
    for ( y = 0; y < oSizeY; y++) {
      // project y to old coordinates
      origY = (int)(y * m_factorY);
      filterX = 0;
      for ( x = 0; x < oSizeX; x++) {
        // project x to old coordinates
        origX = (int)(x * m_factorX);
        pOutput = output + oSizeX * y + x;
        *pOutput = filter( input, &m_filterIntCoeffsX[filterX], &m_filterIntCoeffsY[filterY], m_filterPrecision, origX, origY, iSizeX,  vMin, vMax);
        filterX += m_filterTapsX;
      }
      filterY += m_filterTapsY;
    }    
  }
}


void FrameScale::filter( imgpel *input, imgpel *output, int iSizeX, int iSizeY, int oSizeX, int oSizeY, int vMin, int vMax )
{
  /* we assume the input and output images are stored in raster scan mode */
  int x, y, filterX, filterY = 0;
  
  int origY, origX;
  imgpel *pOutput = output;
  
  m_iMaxX = iSizeX - 1;
  m_iMaxY = iSizeY - 1;

  for ( y = 0; y < oSizeY; y++) {
    // project y to old coordinates
    origY = (int)(y * m_factorY);
    filterX = 0;
    for ( x = 0; x < oSizeX; x++) {
      // project x to old coordinates
      origX = (int)(x * m_factorX);
      pOutput = output + oSizeX * y + x;
      *pOutput = filter( input, &m_filterCoeffsX[filterX], &m_filterCoeffsY[filterY], origX, origY, iSizeX,  vMin, vMax);
      filterX += m_filterTapsX;
    }
    filterY += m_filterTapsY;
  }
}

void FrameScale::filter( imgpel *input, imgpel *output, int iSizeX, int iSizeY, int oSizeX, int oSizeY, int vMin, int vMax, bool isChroma )
{
  /* we assume the input and output images are stored in raster scan mode */
  int x, y, filterX, filterY = 0;
  
  int origY, origX;
  imgpel *pOutput = output;
  
  m_iMaxX = iSizeX - 1;
  m_iMaxY = iSizeY - 1;
  
  if (isChroma) {
    for ( y = 0; y < oSizeY; y++) {
      // project y to old coordinates
      origY = (int)(y * m_factorY);
      filterX = 0;
      for ( x = 0; x < oSizeX; x++) {
        // project x to old coordinates
        origX = (int)(x * m_factorX);
        pOutput = output + oSizeX * y + x;
        *pOutput = filter( input, 
                          &m_chromaFilterIntCoeffsX[filterX],
                          &m_chromaFilterIntCoeffsY[filterY],
                          m_chromaFilterTapsX,
                          m_chromaFilterTapsY,
                          &m_chromaFilterOffsetsX[filterX],
                          &m_chromaFilterOffsetsY[filterY], 
                          origX, origY, iSizeX,  vMin, vMax);
        filterX += m_chromaFilterTapsX;
      }
      filterY += m_chromaFilterTapsY;
    }
  }
  else {
    for ( y = 0; y < oSizeY; y++) {
      // project y to old coordinates
      origY = (int)(y * m_factorY);
      filterX = 0;
      for ( x = 0; x < oSizeX; x++) {
        // project x to old coordinates
        origX = (int)(x * m_factorX);
        pOutput = output + oSizeX * y + x;
        *pOutput = filter( input,
                          &m_filterIntCoeffsX[filterX],
                          &m_filterIntCoeffsY[filterY], 
                          m_filterTapsX,
                          m_filterTapsY,
                          &m_filterOffsetsX[filterX],
                          &m_filterOffsetsY[filterY], 
                          origX, origY, iSizeX,  vMin, vMax);
        
        //printf("Start values %d %d %d %d (%d %d)\n", m_filterIntCoeffsX[filterX], m_filterOffsetsX[filterX], m_filterOffsetsX[filterX],m_filterOffsetsY[filterY], filterX, filterY);
        filterX += m_filterTapsX;
      }
      filterY += m_filterTapsY;
    }
  }
}

void FrameScale::filter( uint16 *input, uint16 *output, int iSizeX, int iSizeY, int oSizeX, int oSizeY, int vMin, int vMax )
{
  /* we assume the input and output images are stored in raster scan mode */
  int x, y, filterX, filterY = 0;
  
  int origY, origX;
  uint16 *pOutput = output;
  
  m_iMaxX = iSizeX - 1;
  m_iMaxY = iSizeY - 1;
  
  for ( y = 0; y < oSizeY; y++) {
    // project y to old coordinates
    origY = (int)(y * m_factorY);
    filterX = 0;
    for ( x = 0; x < oSizeX; x++) {
      // project x to old coordinates
      origX = (int)(x * m_factorX);
      pOutput = output + oSizeX * y + x;
      *pOutput = filter( input, &m_filterCoeffsX[filterX], &m_filterCoeffsY[filterY], origX, origY, iSizeX,  vMin, vMax);
      filterX += m_filterTapsX;
    }
    filterY += m_filterTapsY;
  }
}

void FrameScale::filter( uint16 *input, uint16 *output, int iSizeX, int iSizeY, int oSizeX, int oSizeY, int vMin, int vMax, bool isChroma )
{
  /* we assume the input and output images are stored in raster scan mode */
  int x, y, filterX, filterY = 0;
  
  int origY, origX;
  uint16 *pOutput = output;
  
  m_iMaxX = iSizeX - 1;
  m_iMaxY = iSizeY - 1;
  
  if (isChroma) {
    for ( y = 0; y < oSizeY; y++) {
      // project y to old coordinates
      origY = (int)(y * m_factorY);
      //printf("(%d:%d) ", origY, y);
      filterX = 0;
      for ( x = 0; x < oSizeX; x++) {
        // project x to old coordinates
        origX = (int)(x * m_factorX);
        //printf(" %d ", origX);
        pOutput = output + oSizeX * y + x;
        *pOutput = filter( input, 
                          &m_chromaFilterIntCoeffsX[filterX],
                          &m_chromaFilterIntCoeffsY[filterY],
                          m_chromaFilterTapsX,
                          m_chromaFilterTapsY,
                          &m_chromaFilterOffsetsX[filterX],
                          &m_chromaFilterOffsetsY[filterY], 
                          origX, origY, iSizeX,  vMin, vMax);
        filterX += m_chromaFilterTapsX;
      }
      filterY += m_chromaFilterTapsY;
      //printf("\n");
    }
    //printf("\n");
  }
  else {
    for ( y = 0; y < oSizeY; y++) {
      // project y to old coordinates
      origY = (int)(y * m_factorY);
      filterX = 0;
      for ( x = 0; x < oSizeX; x++) {
        // project x to old coordinates
        origX = (int)(x * m_factorX);
        pOutput = output + oSizeX * y + x;
        *pOutput = filter( input,
                          &m_filterIntCoeffsX[filterX],
                          &m_filterIntCoeffsY[filterY], 
                          m_filterTapsX,
                          m_filterTapsY,
                          &m_filterOffsetsX[filterX],
                          &m_filterOffsetsY[filterY], 
                          origX, origY, iSizeX,  vMin, vMax);
        //printf("Start values %d %d %d %d (%d %d)\n", m_filterIntCoeffsX[filterX], m_filterOffsetsX[filterX], m_filterOffsetsX[filterX],m_filterOffsetsY[filterY], filterX, filterY);

        filterX += m_filterTapsX;
      }
      filterY += m_filterTapsY;
    }
  }
}


void FrameScale::filter( float *input, float *output, int iSizeX, int iSizeY, int oSizeX, int oSizeY, double vMin, double vMax )
{
  /* we assume the input and output images are stored in raster scan mode */
  int x, y, filterX, filterY = 0;
  
  int origY, origX;
  float *pOutput = output;
  
  m_iMaxX = iSizeX - 1;
  m_iMaxY = iSizeY - 1;
  
  for ( y = 0; y < oSizeY; y++) {
    // project y to old coordinates
    origY = (int)(y * m_factorY + m_offsetY);
    filterX = 0;
    for ( x = 0; x < oSizeX; x++) {
      // project x to old coordinates
      origX = (int)(x * m_factorX + m_offsetX);
      pOutput = output + oSizeX * y + x;
      *pOutput = filter( input, &m_filterCoeffsX[filterX], &m_filterCoeffsY[filterY], origX, origY, iSizeX,  vMin, vMax);
      filterX += m_filterTapsX;
    }
    filterY += m_filterTapsY;
  }
}
} // namespace hdrtoolslib

//-----------------------------------------------------------------------------
// End of file
//-----------------------------------------------------------------------------
