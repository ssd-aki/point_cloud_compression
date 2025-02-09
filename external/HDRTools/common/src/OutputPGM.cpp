/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * <OWNER> = Apple Inc.
 * <ORGANIZATION> = Apple Inc.
 * <YEAR> = 2018
 *
 * Copyright (c) 2018, Apple Inc.
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
 * \file OutputPGM.cpp
 *
 * \brief
 *    OutputPGM class C++ file for allowing output of PGM files
 *
 * \author
 *     - Alexis Michael Tourapis         <atourapis@apple.com>
 *
 *************************************************************************************
 */

//-----------------------------------------------------------------------------
// Include headers
//-----------------------------------------------------------------------------

#include <string.h>
#include "OutputPGM.H"
#include "Global.H"
#include "IOFunctions.H"

//-----------------------------------------------------------------------------
// Macros/Defines
//-----------------------------------------------------------------------------

namespace hdrtoolslib {

//-----------------------------------------------------------------------------
// Constructor/destructor
//-----------------------------------------------------------------------------


OutputPGM::OutputPGM(IOVideo *videoFile, FrameFormat *format) {
  // We currently do not support floating point data
  m_isFloat         = FALSE;
  format->m_isFloat = m_isFloat;
  videoFile->m_format.m_isFloat = m_isFloat;
  m_format          = *format;
  m_frameRate       = format->m_frameRate;
  auto PQ = getFraction(m_frameRate);
  m_frameRateNum    = PQ.first;
  m_frameRateDenom  = PQ.second;

  m_memoryAllocated = FALSE;
  
  m_comp[Y_COMP]      = NULL;
  m_comp[U_COMP]      = NULL;
  m_comp[V_COMP]      = NULL;
  
  m_floatComp[Y_COMP] = NULL;
  m_floatComp[U_COMP] = NULL;
  m_floatComp[V_COMP] = NULL;
  m_floatComp[A_COMP] = NULL;
  
  m_ui16Comp[Y_COMP] = NULL;
  m_ui16Comp[U_COMP] = NULL;
  m_ui16Comp[V_COMP] = NULL;
  m_ui16Comp[A_COMP] = NULL;
  
  allocateMemory(format);
  
}

OutputPGM::~OutputPGM() {
  
  freeMemory();
  
  clear();
}

//-----------------------------------------------------------------------------
// Private methods
//-----------------------------------------------------------------------------

/*!
 ************************************************************************
 * \brief
 *   Set an unsigned short without swapping.
 *
 ************************************************************************
 */
static uint32 setU16 (PGraphics * t, uint16 in)
{
  char *pIn = (char *) &in;
  *t->mp++ = *pIn++;
  *t->mp++ = *pIn++;
  
  return 2;
}


/*!
 ************************************************************************
 * \brief
 *   Set an unsigned int32 without swapping.
 *
 ************************************************************************
 */
static uint32 setU32 (PGraphics * t, uint32 in)
{
  char *pIn = (char *) &in;
  *t->mp++ = *pIn++;
  *t->mp++ = *pIn++;
  *t->mp++ = *pIn++;
  *t->mp++ = *pIn++;
  
  return 4;
}


// Swap versions

/*!
 ************************************************************************
 * \brief
 *   Set an unsigned short and swap.
 *
 ************************************************************************
 */
static uint32 setSwappedU16 (PGraphics * t, uint16 in)
{
  char *pIn = (char *) &in;
  *t->mp++ = pIn[1];
  *t->mp++ = pIn[0];
  
  return 2;
}


/*!
 ************************************************************************
 * \brief
 *   Set an unsigned int32 and swap.
 *
 ************************************************************************
 */
static uint32 setSwappedU32 (PGraphics * t, uint32 in)
{
  char *pIn = (char *) &in;
  *t->mp++ = pIn[3];
  *t->mp++ = pIn[2];
  *t->mp++ = pIn[1];
  *t->mp++ = pIn[0];
  
  return 2;
}

/*!
 ************************************************************************
 * \brief
 *   Write writeFileFromMemory into file named 'path' into memory buffer 't->fileInMemory'.
 *
 * \return
 *   0 if successful
 ************************************************************************
 */
int OutputPGM::writeFileFromMemory (PGraphics * t, FILE *file, uint32 counter)
{
  long result = (long) fwrite((char *) &t->fileInMemory[0], sizeof(char), counter, file);
  if (result != counter) {
    if (file != NULL) {
      fclose( file);
      file = NULL;
    }
    return 1;
  }
  
  return 0;
}

/*!
 ************************************************************************
 * \brief
 *    Read image data into 't->img'.
 *
 ************************************************************************
 */
uint32 OutputPGM::writeImageData (PGraphics * t)
{
  uint32  i;
  uint8  *mp, *s;
  uint16 *p;
  uint32 byteCounter = 0;
  
  switch (t->BitsPerSample[0]) {
    case 8:
      p = (uint16 *) &t->img[0];
      s = (uint8 *) &t->fileInMemory[0];
      for (i=0; i < m_pgm.m_height * m_pgm.m_width; ++i) {
        byteCounter ++;
        *s++ = (uint8) *p++;
      }
      break;
    case 16:
      mp = t->mp;                       // save memory pointer
      p = (uint16 *) &t->img[0];
      for (i = 0; i < m_pgm.m_height * m_pgm.m_width; ++i) {
          byteCounter += t->setU16( t, *p++);
      }
     
      t->mp = mp;                       // restore memory pointer
      break;
  }
  return byteCounter;
}

/*!
 *****************************************************************************
 * \brief
 *    Read the Image File Header.
 *
 *****************************************************************************
 */
uint32 OutputPGM::writeImageFileHeader (FrameFormat *format, FILE* file)
{
  uint32 byteCounter = 0;
  fprintf(file, "P5\n");
  fprintf(file, "%d %d\n", m_width[Y_COMP], m_height[Y_COMP]);
  fprintf(file, "%d\n", (1 << m_bitDepthComp[Y_COMP]) - 1);
  byteCounter = (uint32) ftell(file);
  return byteCounter;
}

/*!
 *****************************************************************************
 * \brief
 *    Write the PGM file.
 *
 *****************************************************************************
 */
int OutputPGM::writeFile (FrameFormat *format, FILE* file) {
  
  if (m_memoryAllocated == FALSE) {
    allocateMemory(format);
  }
  
  writeImageFileHeader( format, file);
  
  if (writeFileFromMemory( &m_pgm, file, (uint32) (maxFramePosition)))
    goto Error;
  
  return 1;
  
Error:
  return 0;
}


/*!
 ************************************************************************
 * \brief
 *    Open file containing a single frame
 ************************************************************************
 */
int OutputPGM::openFrameFile( IOVideo *outputFile, char *outFile, int FrameNumberInFile, size_t outSize)
{
  char outNumber[16];
  int length = 0;
  outNumber[length]='\0';
  length = (int) strlen(outputFile->m_fHead);
  strncpy(outFile, outputFile->m_fHead, outSize);
  outFile[length]='\0'; 
  
  // Is this a single frame file? If yes, m_fTail would be of size 0.
  if (strlen(outputFile->m_fTail) != 0) {
    if (outputFile->m_zeroPad == TRUE)
      snprintf(outNumber, 16, "%0*d", outputFile->m_numDigits, FrameNumberInFile);
    else
      snprintf(outNumber, 16, "%*d", outputFile->m_numDigits, FrameNumberInFile);
    
    strncat(outFile, outNumber, strlen(outFile) - 1);
    length += sizeof(outNumber);
    outFile[length]='\0';
    strncat(outFile, outputFile->m_fTail, strlen(outFile) - 1);
    length += (int) strlen(outputFile->m_fTail);
    outFile[length]='\0';
  }
  
  
  //*file = IOFunctions::openFile(outFile, "w+t");
  
  return outputFile->m_fileNum;
}

void OutputPGM::allocateMemory(FrameFormat *format)
{
  m_memoryAllocated = TRUE;
  m_chromaFormat     = format->m_chromaFormat;
  if (m_chromaFormat != CF_400) {
    printf("PGM files can only be in a 400/mono format. Error!\n");
    exit(EXIT_FAILURE);
  }
  
  m_pgm.m_height = m_height[Y_COMP] = format->m_height[Y_COMP];
  m_pgm.m_width  = m_width[Y_COMP]  = format->m_width [Y_COMP];
  
  m_width [U_COMP] = m_width [V_COMP] = format->m_height[U_COMP] = format->m_height[V_COMP] = 0;
  m_height[U_COMP] = m_height[V_COMP] = format->m_width[U_COMP] = format->m_width[V_COMP] = 0;
  m_height [A_COMP] = 0;
  m_width  [A_COMP] = 0;
  
  m_compSize[Y_COMP] = format->m_compSize[Y_COMP] = m_height[Y_COMP] * m_width[Y_COMP];
  m_compSize[U_COMP] = format->m_compSize[U_COMP] = m_height[U_COMP] * m_width[U_COMP];
  m_compSize[V_COMP] = format->m_compSize[V_COMP] = m_height[V_COMP] * m_width[V_COMP];
  m_compSize[A_COMP] = format->m_compSize[A_COMP] = m_height[A_COMP] * m_width[A_COMP];
  
  
  m_size = format->m_size = m_compSize[Y_COMP] + m_compSize[U_COMP] + m_compSize[V_COMP] + m_compSize[A_COMP];
  
  m_colorSpace       = format->m_colorSpace;
  m_colorPrimaries   = format->m_colorPrimaries;
  m_sampleRange      = format->m_sampleRange;
  
  m_transferFunction = format->m_transferFunction;
  m_systemGamma      = format->m_systemGamma;
  
  m_bitDepthComp[Y_COMP] = (m_sampleRange == SR_SDI_SCALED && format->m_bitDepthComp[Y_COMP] > 8) ? 16 : format->m_bitDepthComp[Y_COMP];
  m_bitDepthComp[U_COMP] = (m_sampleRange == SR_SDI_SCALED && format->m_bitDepthComp[U_COMP] > 8) ? 16 : format->m_bitDepthComp[U_COMP];
  m_bitDepthComp[V_COMP] = (m_sampleRange == SR_SDI_SCALED && format->m_bitDepthComp[U_COMP] > 8) ? 16 : format->m_bitDepthComp[V_COMP];
  
  m_pgm.BitsPerSample[Y_COMP] = (m_bitDepthComp[Y_COMP] > 8 ? 16 : 8);
  m_pgm.BitsPerSample[U_COMP] = (m_bitDepthComp[U_COMP] > 8 ? 16 : 8);
  m_pgm.BitsPerSample[V_COMP] = (m_bitDepthComp[V_COMP] > 8 ? 16 : 8);
  
  
  m_bitDepthComp[A_COMP] = format->m_bitDepthComp[A_COMP];
    
  m_isInterleaved = FALSE;
  m_isInterlaced  = FALSE;
  
  m_chromaLocation[FP_TOP]    = format->m_chromaLocation[FP_TOP];
  m_chromaLocation[FP_BOTTOM] = format->m_chromaLocation[FP_BOTTOM];
  
  if (m_isInterlaced == FALSE && m_chromaLocation[FP_TOP] != m_chromaLocation[FP_BOTTOM]) {
    printf("Progressive Content. Chroma Type Location needs to be the same for both fields.\n");
    printf("Resetting Bottom field chroma location from type %d to type %d\n", m_chromaLocation[FP_BOTTOM], m_chromaLocation[FP_TOP]);
    m_chromaLocation[FP_BOTTOM] = format->m_chromaLocation[FP_BOTTOM] = m_chromaLocation[FP_TOP];
  }
  
  
  // init size of file based on image data to write (without headers
  m_pgmSize = m_size * (m_pgm.BitsPerSample[Y_COMP] > 8 ? 2 : 1);
    
  // memory buffer for the image data
  m_pgm.img.resize((unsigned int) m_pgmSize);
  // Assign a rather large buffer
  m_pgm.fileInMemory.resize((long) m_size * 4);
  
  if (format->m_bitDepthComp[Y_COMP] == 8) {
    m_data.resize((unsigned int) m_size);
    m_comp[Y_COMP]      = &m_data[0];
    m_comp[U_COMP]      = m_comp[Y_COMP] + m_compSize[Y_COMP];
    m_comp[V_COMP]      = m_comp[U_COMP] + m_compSize[U_COMP];
    m_comp[A_COMP]      = NULL;
    m_ui16Comp[Y_COMP]  = NULL;
    m_ui16Comp[U_COMP]  = NULL;
    m_ui16Comp[V_COMP]  = NULL;
    m_ui16Comp[A_COMP]  = NULL;
  }
  else {
    m_comp[Y_COMP]      = NULL;
    m_comp[U_COMP]      = NULL;
    m_comp[V_COMP]      = NULL;
    m_ui16Data.resize((unsigned int) m_size);
    m_ui16Comp[Y_COMP]  = &m_ui16Data[0];
    m_ui16Comp[U_COMP]  = m_ui16Comp[Y_COMP] + m_compSize[Y_COMP];
    m_ui16Comp[V_COMP]  = m_ui16Comp[U_COMP] + m_compSize[U_COMP];
    m_ui16Comp[A_COMP]  = NULL;
  }
  
  m_floatComp[Y_COMP] = NULL;
  m_floatComp[U_COMP] = NULL;
  m_floatComp[V_COMP] = NULL;
  m_floatComp[A_COMP] = NULL;
  
  int endian = 1;
  int machineLittleEndian = (*( (char *)(&endian) ) == 1) ? 1 : 0;
  
  m_pgm.le = 1;
  if (m_pgm.le == machineLittleEndian)  { // endianness of machine matches file
    m_pgm.setU16 = setSwappedU16;
    m_pgm.setU32 = setSwappedU32;
  }
  else {                               // endianness of machine does not match file
    m_pgm.setU16 = setU16;
    m_pgm.setU32 = setU32;
  }
  m_pgm.mp = (uint8 *) &m_pgm.fileInMemory[0];
  
}

void OutputPGM::freeMemory()
{
  m_comp[Y_COMP]     = NULL;
  m_comp[U_COMP]     = NULL;
  m_comp[V_COMP]     = NULL;
  m_comp[A_COMP]     = NULL;
  
  m_ui16Comp[Y_COMP] = NULL;
  m_ui16Comp[U_COMP] = NULL;
  m_ui16Comp[V_COMP] = NULL;
  m_ui16Comp[A_COMP] = NULL;
  
  m_floatComp[Y_COMP] = NULL;
  m_floatComp[U_COMP] = NULL;
  m_floatComp[V_COMP] = NULL;
  m_floatComp[A_COMP] = NULL;
}

/*!
 ************************************************************************
 * \brief
 *    Read Header data
 ************************************************************************
 */

int OutputPGM::writeData (int vfile,  FrameFormat *source, uint8 *buf) {
  return 1;
}

int OutputPGM::reformatData () {
  int i, k;
  
  if (m_pgm.BitsPerSample[0] == 8) {
    imgpel *comp0 = NULL;
    
    uint16 *curBuf = (uint16 *) &m_pgm.img[0];
    
    // Unpack the data appropriately (interleaving is done at the row level).
    for (k = 0; k < m_height[Y_COMP]; k++) {
      comp0   = &m_comp[0][k * m_width[0]];
      for (i = 0; i < m_width[Y_COMP]; i++) {
        *curBuf++ = *comp0++;
      }
    }
    maxFramePosition =m_height[Y_COMP] * m_width[Y_COMP];
  }
  else {
    uint16 *comp0 = NULL;
    
    uint16 *curBuf = (uint16 *) &m_pgm.img[0];
    
    // Unpack the data appropriately (interleaving is done at the row level).
    for (k = 0; k < m_height[Y_COMP]; k++) {
      comp0   = &m_ui16Comp[0][k * m_width[0]];
      for (i = 0; i < m_width[Y_COMP]; i++) {
        *curBuf++ = *comp0++;
      }
    }
        maxFramePosition =m_height[Y_COMP] * m_width[Y_COMP] * 2;
  }
  return 1;
}


//-----------------------------------------------------------------------------
// Public methods
//-----------------------------------------------------------------------------

/*!
 ************************************************************************
 * \brief
 *    Writes one new frame into a single PGM file
 *
 * \param inputFile
 *    Output file to write into
 * \param frameNumber
 *    Frame number in the source file
 * \param fileHeader
 *    Number of bytes in the source file to be skipped
 * \param frameSkip
 *    Start position in file
 ************************************************************************
 */
int OutputPGM::writeOneFrame (IOVideo *outputFile, int frameNumber, int fileHeader, int frameSkip) {
  char outFile [FILE_NAME_SIZE];
  FILE*         frameFile = NULL;

  int fileWrite = 1;
  FrameFormat *format = &outputFile->m_format;
  openFrameFile( outputFile, outFile, frameNumber + frameSkip, sizeof(outFile));
  
  frameFile = IOFunctions::openFile(outFile, "w+t");

  if (frameFile != NULL) {
    
    reformatData ();
    writeImageData( &m_pgm);

    fileWrite = writeFile( format, frameFile);
    
    IOFunctions::closeFile(frameFile);
  }
  
  return fileWrite;
}
} // namespace hdrtoolslib
//-----------------------------------------------------------------------------
// End of file
//-----------------------------------------------------------------------------

