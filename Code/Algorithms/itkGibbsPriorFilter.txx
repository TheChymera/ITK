/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    itkGibbsPriorFilter.txx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2001 Insight Consortium
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * The name of the Insight Consortium, nor the names of any consortium members,
   nor of any contributors, may be used to endorse or promote products derived
   from this software without specific prior written permission.

  * Modified source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#ifndef _itkGibbsPriorFilter_txx
#define _itkGibbsPriorFilter_txx


#include "itkGibbsPriorFilter.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

//typedef itk::Mesh<int>  Mesh;

namespace itk
{
  
/**
 * set intial value of some parameters in the constructor
 */

template <typename TInputImage, typename TClassifiedImage>
GibbsPriorFilter<TInputImage, TClassifiedImage>
::GibbsPriorFilter()
{
// Modify superclass default values, set default values,
// can be overridden by subclasses
  m_BoundaryGradient = 7;
  m_GibbsNeighborsThreshold = 1;
  m_BoundaryWeight = 1;
  m_GibbsPriorWeight = 1;
  m_StartPoint[0] = 128;
  m_StartPoint[1] = 128;
  m_StartPoint[2] = 0;
  m_StartRadius = 10;
}

/**
 * Set the labelled image.
 */
template<typename TInputImage, typename TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::SetLabelledImage(LabelledImageType image)
{
  m_LabelledImage = image;
  this->Allocate();
}// Set the LabelledImage

/**
 * GenerateInputRequestedRegion method.
 */
template <class TInputImage, class TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::GenerateInputRequestedRegion()
{
  // this filter requires the all of the input image to be in
  // the buffer
  InputImageType inputPtr = this->GetInput();
  inputPtr->SetRequestedRegionToLargestPossibleRegion();
}


/**
 * EnlargeOutputRequestedRegion method.
 */
template <class TInputImage, class TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::EnlargeOutputRequestedRegion(
DataObject *output )
{

  // this filter requires the all of the output image to be in
  // the buffer
  TClassifiedImage *imgData;
  imgData = dynamic_cast<TClassifiedImage*>( output );
  imgData->SetRequestedRegionToLargestPossibleRegion();

}

/**
 * GenerateOutputInformation method.
 */
template <class TInputImage, class TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::GenerateOutputInformation()
{

  typename TInputImage::Pointer input = this->GetInput();
  typename TClassifiedImage::Pointer output = this->GetOutput();
  output->SetLargestPossibleRegion( input->GetLargestPossibleRegion() );

}

/**
 * allocate the memeory for classified image.
 */
template<class TInputImage, class TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::Allocate()
{
  if( m_NumberOfClasses <= 0 )
    {
    throw ExceptionObject(__FILE__, __LINE__);
    }

  InputImageSizeType inputImageSize = m_InputImage->GetBufferedRegion().GetSize();

  //Ensure that the data provided is three dimensional data set
  if(TInputImage::ImageDimension <= 2 )
    {
    throw ExceptionObject(__FILE__, __LINE__);
    }
  
  //---------------------------------------------------------------------
  //Get the image width/height and depth
  //---------------------------------------------------------------------       
  m_ImageWidth  = static_cast<int>(inputImageSize[0]);
  m_ImageHeight = static_cast<int>(inputImageSize[1]);
  m_ImageDepth  = static_cast<int>(inputImageSize[2]);
 
  m_LabelStatus = new unsigned int[m_ImageWidth*m_ImageHeight*m_ImageDepth]; 
  for( int index = 0; 
       index < ( m_ImageWidth * m_ImageHeight * m_ImageDepth ); 
       index++ ) 
    {
    m_LabelStatus[index]=1;
    }

}// Allocate

template <typename TInputImage, typename TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::SetStartPoint (int x, int y, int z) 
{ 
  m_StartPoint[0] = x; 
  m_StartPoint[1] = y; 
  m_StartPoint[2] = z; 
} 

/*
template <typename TInputImage, typename TClassifiedImage>
int
GibbsPriorFilter<TInputImage, TClassifiedImage>
::Mini(int i)
{
  int j, low, f[5];

  InputImageIterator  inputImageIt(m_InputImage, 
                                   m_InputImage->GetBufferedRegion() );

  LabelledImageIndexType offsetIndex3D = {0, 0, 0};

  int size = m_ImageWidth * m_ImageHeight * m_ImageDepth;
  int frame = m_ImageWidth * m_ImageHeight;
  int rowsize = m_ImageWidth;

  offsetIndex3D[2] = i / frame;
  offsetIndex3D[1] = (i % frame) / m_ImageHeight;
  offsetIndex3D[0] = (i % frame) % m_ImageHeight;

  if ((i > rowsize - 1)&&((i%rowsize) != rowsize - 1)&&(i < size - rowsize)&&((i%rowsize) != 0)) {
//    inputImageIt = inputImageIt - rowsize;
    offsetIndex3D[1]--;
  f[0] = (int) m_InputImage->GetPixel( offsetIndex3D )[0];
//  inputImageIt = inputImageIt + rowsize - 1;
    offsetIndex3D[0]--;
    offsetIndex3D[1]++;
    f[1] = (int) m_InputImage->GetPixel( offsetIndex3D )[0];
//  inputImageIt = inputImageIt + 2;
    offsetIndex3D[0]++;
  f[2] = (int) m_InputImage->GetPixel( offsetIndex3D )[0];
    offsetIndex3D[0]++;
    f[3] = (int) m_InputImage->GetPixel( offsetIndex3D )[0];
//  inputImageIt = inputImageIt + rowsize - 1;
    offsetIndex3D[0]--;
    offsetIndex3D[1]++;
    f[4] = (int) m_InputImage->GetPixel( offsetIndex3D )[0];
  }

  low = f[0];
  for (j = 1; j < 5; j++) {
    if (low > f[j]) low = f[j];
  }
  return low;
}

template <typename TInputImage, typename TClassifiedImage>
int
GibbsPriorFilter<TInputImage, TClassifiedImage>
::Maxi(int i)
{
  int j, high, f[5];

  InputImageIterator  inputImageIt(m_InputImage, 
                                   m_InputImage->GetBufferedRegion() );

  LabelledImageIndexType offsetIndex3D = { 0, 0, 0};

  int size = m_ImageWidth * m_ImageHeight * m_ImageDepth;
  int frame = m_ImageWidth * m_ImageHeight;
  int rowsize = m_ImageWidth;

  offsetIndex3D[2] = i / frame;
  offsetIndex3D[1] = (i % frame) / m_ImageHeight;
  offsetIndex3D[0] = (i % frame) % m_ImageHeight;

  if ((i > rowsize - 1)&&((i%rowsize) != rowsize - 1)&&(i < size - rowsize)&&((i%rowsize) != 0)) {

    offsetIndex3D[1]--;
  f[0] = (int) m_InputImage->GetPixel( offsetIndex3D )[0];

    offsetIndex3D[0]--;
    offsetIndex3D[1]++;
    f[1] = (int) m_InputImage->GetPixel( offsetIndex3D )[0];

    offsetIndex3D[0]++;
  f[2] = (int) m_InputImage->GetPixel( offsetIndex3D )[0];

    offsetIndex3D[0]++;
    f[3] = (int) m_InputImage->GetPixel( offsetIndex3D )[0];

    offsetIndex3D[0]--;
    offsetIndex3D[1]++;
    f[4] = (int) m_InputImage->GetPixel( offsetIndex3D )[0];
  }


  high = f[0];
  for (j = 1; j < 5; j++) {
    if (high < f[j]) high = f[j];
  }
  
  return high;
}
*/

template <typename TInputImage, typename TClassifiedImage>
int
GibbsPriorFilter<TInputImage, TClassifiedImage>
::GreyScalarBoundary(LabelledImageIndexType Index3D)
{
  int i, change, signs[4]={0,0,0,0}, x, numx;
  int lowpoint;
  int origin, neighbors[4];

  origin = (int) m_InputImage->GetPixel( Index3D )[0];
  int j = 0;
  for(int i = 0; i < ImageDimension-1; i++) 
    {
    Index3D[i]--;
    neighbors[j] = (int) m_InputImage->GetPixel( Index3D )[0];
    Index3D[i]++;
    j++;

    Index3D[i]++;
    neighbors[j] = (int) m_InputImage->GetPixel( Index3D )[0];
    Index3D[i]--;
    j++;
    }

// calculate the minimum points of piecewise smoothness  
  lowpoint = origin;
  change = 1;
  x = origin;
  numx = 1;
  while ( change > 0 ) 
    {
    change = 0;
    for (i=0; i<4; i++) 
      {
      if (signs[i] == 0) 
        {
        if (abs(lowpoint - neighbors[i]) < m_BoundaryGradient) 
          {
          numx++;
          x += neighbors[i];
          signs[i]++;
          change++;
          }
        }
      }

    lowpoint = x/numx;

    for (i=0; i<4; i++) 
      {
      if (signs[i] == 1) 
        {
        if (abs(lowpoint - neighbors[i]) > m_BoundaryGradient) 
          {
          numx--;
          x -= neighbors[i];
          signs[i]--;
          change++;
          }
        }
      }
    lowpoint = x/numx;
    }

  return lowpoint;
}

template<typename TInputImage, typename TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::SetClassifier( typename ClassifierType::Pointer ptrToClassifier )
{
  if( ( ptrToClassifier == 0 ) || (m_NumberOfClasses <= 0) )
    throw ExceptionObject(__FILE__, __LINE__);

  m_ClassifierPtr = ptrToClassifier;
  m_ClassifierPtr->SetNumberOfClasses( m_NumberOfClasses );
}//end SetClassifier

/*  
template <typename TInputImage, typename TClassifiedImage>
float
GibbsPriorFilter<TInputImage, TClassifiedImage>
::GradientEnergy(InputImageVectorType fi, int i)
{
  float energy, x1, x2;
  int j, f[4], di, dif;
  
  InputImageIterator  inputImageIt(m_InputImage, 
                                   m_InputImage->GetBufferedRegion() );

  LabelledImageIndexType offsetIndex3D = { 0, 0, 0};

  int size = m_ImageWidth * m_ImageHeight * m_ImageDepth;
  int frame = m_ImageWidth * m_ImageHeight;
  int rowsize = m_ImageWidth;

  offsetIndex3D[2] = i / frame;
  offsetIndex3D[1] = (i % frame) / m_ImageHeight;
  offsetIndex3D[0] = (i % frame) % m_ImageHeight;

  di = (int) m_InputImage->GetPixel( offsetIndex3D )[0];

  if ((i > rowsize - 1)&&((i%rowsize) != rowsize - 1)&&(i < size - rowsize)&&((i%rowsize) != 0)) {

    offsetIndex3D[1]--;
  f[0] = (int) m_InputImage->GetPixel( offsetIndex3D )[0];

    offsetIndex3D[0]--;
    offsetIndex3D[1]++;
    f[1] = (int) m_InputImage->GetPixel( offsetIndex3D )[0];

    offsetIndex3D[0]++;
    offsetIndex3D[0]++;
    f[2] = (int) m_InputImage->GetPixel( offsetIndex3D )[0];

    offsetIndex3D[0]--;
    offsetIndex3D[1]++;
    f[3] = (int) m_InputImage->GetPixel( offsetIndex3D )[0];
  }

  dif = 0;
  energy = 0.0;
  x1 = 0;
  for (j = 0; j < 4; j++) {
  if ( abs(f[j]-fi[0]) > m_BoundaryGradient ) dif = 1;
    x1 = x1 + (fi[0] - f[j])*(fi[0] - f[j])*(1 - dif) + dif*m_BoundaryGradient;
  }

  x1 = m_BoundaryWeight*x1;
  x2 = (fi[0] - di)*(fi[0] - di);

  energy = x1 + x2;

  return energy; 
}
*/

template<typename TInputImage, typename TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::SetTrainingImage( TrainingImageType image )
{
  m_TrainingImage = image;
}//end SetTrainingImage

template<typename TInputImage, typename TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::SetBoundaryGradient( int a )
{
  m_BoundaryGradient = a;
}

template <typename TInputImage, typename TClassifiedImage>
int
GibbsPriorFilter<TInputImage, TClassifiedImage>
::Sim(int a, int b)
{
  if (a == b) return 1;
  return 0;
}

/**
 * GibbsTotalEnergy method minimizes the local characteristic item
 * in the energy function.
 */
template <typename TInputImage, typename TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::GibbsTotalEnergy(int i)
{
  LabelledImageIndexType offsetIndex3D = {0, 0, 0};

  int frame = m_ImageWidth * m_ImageHeight;
  int rowsize = m_ImageWidth;

  float energy[2] = {0.0, 0.0}, maxenergy;
  int label, originlabel;
  maxenergy = 1e+20;

  offsetIndex3D[2] = i / frame;
  offsetIndex3D[1] = (i % frame) / m_ImageHeight;
  offsetIndex3D[0] = (i % frame) % m_ImageHeight;

  for(int j = 1; j < m_NumberOfClasses; j++) {
    energy[j-1] += GibbsEnergy(i,       0, j);
  energy[j-1] += GibbsEnergy(i + rowsize + 1, 1, j);
  energy[j-1] += GibbsEnergy(i + rowsize,   2, j);
  energy[j-1] += GibbsEnergy(i + rowsize - 1, 3, j);
  energy[j-1] += GibbsEnergy(i - 1,     4, j);
  energy[j-1] += GibbsEnergy(i - rowsize - 1, 5, j);
  energy[j-1] += GibbsEnergy(i - rowsize,   6, j);
  energy[j-1] += GibbsEnergy(i - rowsize + 1, 7, j);
  energy[j-1] += GibbsEnergy(i + 1,     8, j);  
  }

  for(int j = 1; j < m_NumberOfClasses; j++) {
    if (energy[j-1] < maxenergy) {
    maxenergy = energy[j-1];
    label = j;
    }
  }

  originlabel = m_LabelledImage->GetPixel(offsetIndex3D);
  if (originlabel != label) {
    m_ErrorCounter++;
  m_LabelledImage->SetPixel(offsetIndex3D, label);
  }
}

template <typename TInputImage, typename TClassifiedImage>
float
GibbsPriorFilter<TInputImage, TClassifiedImage>
::GibbsEnergy(int i, int k, int k1)
{
  LabelledImageIterator  
    labelledImageIt(m_LabelledImage, m_LabelledImage->GetBufferedRegion());

  int f[8];
  int j, neighborcount = 0, simnum = 0, difnum = 0, changenum = 0;
  int changeflag;

  LabelledImageIndexType offsetIndex3D = { 0, 0, 0};
  LabelledImagePixelType labelledPixel;

  int size = m_ImageWidth * m_ImageHeight * m_ImageDepth;
  int frame = m_ImageWidth * m_ImageHeight;
  int rowsize = m_ImageWidth;

  offsetIndex3D[2] = i / frame;
  offsetIndex3D[1] = (i % frame) / m_ImageHeight;
  offsetIndex3D[0] = (i % frame) % m_ImageHeight;
  
  if (k != 0) labelledPixel = 
    ( LabelledImagePixelType ) m_LabelledImage->GetPixel( offsetIndex3D );

  if ((i > rowsize - 1)&&((i%rowsize) != rowsize - 1)&&
    (i < size - rowsize)&&((i%rowsize) != 0)) {

  offsetIndex3D[0]--;
  offsetIndex3D[1]--;
  f[neighborcount++] = (int)m_LabelledImage->GetPixel( offsetIndex3D );
  
  offsetIndex3D[0]++;
  f[neighborcount++] = (int)m_LabelledImage->GetPixel( offsetIndex3D );
  
  offsetIndex3D[0]++;
  f[neighborcount++] = (int)m_LabelledImage->GetPixel( offsetIndex3D );
  
  offsetIndex3D[1]++;  
  f[neighborcount++] = (int)m_LabelledImage->GetPixel( offsetIndex3D );
  
  offsetIndex3D[1]++;
  f[neighborcount++] = (int)m_LabelledImage->GetPixel( offsetIndex3D );
  
  offsetIndex3D[0]--;
  f[neighborcount++] = (int)m_LabelledImage->GetPixel( offsetIndex3D );
  
  offsetIndex3D[0]--;
  f[neighborcount++] = (int)m_LabelledImage->GetPixel( offsetIndex3D );
  
  offsetIndex3D[1]--;
  f[neighborcount++] = (int)m_LabelledImage->GetPixel( offsetIndex3D );

  }

// pixels at the edge of image will be dropped 
  if (neighborcount != 8) return 0.0; 

  if (k != 0) f[k-1] = k1;
  else labelledPixel = k1;
  
  changeflag = (f[0] == labelledPixel)?1:0;

// assuming we are segmenting objects with smooth boundaries, we give 
// weight to such kind of local characteristics
  for(j=0;j<8;j++) {
  if (Sim(f[j], labelledPixel) != changeflag) {
      changenum++;
    changeflag = 1 - changeflag;
  }
      
    if (changeflag) simnum++;
    else difnum++;
  }
  
  if (changenum < 3) {
    if ((simnum==4)||(simnum==5)) return -5.0;
  }
  
  if ((difnum==7)||(difnum==8)||(difnum==6)) return 5.0;

  if (simnum == 8) return -5.0;

  return 0;
}

template<class TInputImage, class TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::Advance()
{
  this->GenerateData();
}

template<class TInputImage, class TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::GenerateData()
{
  //First run the Gaussian classifier calculator and
  //generate the Gaussian model for the different classes
  //and then generate the initial labelled dataset.
  m_InputImage = this->GetInput();
    
  //Give the input image and training image set to the  
  //classifier, for the first iteration, use the original image
  //in the following loops, use the result provided by the 
  //deformable model
  if (m_RecursiveNum == 0) 
    {
    m_ClassifierPtr->SetInputImage( m_InputImage );
    // create the training image using the original image
    m_ClassifierPtr->SetTrainingImage( m_TrainingImage );
    } 
  else 
    {
    m_ClassifierPtr->SetInputImage( m_InputImage );
    // create the training image using deformable model
    m_ClassifierPtr->SetTrainingImage( m_TrainingImage );
  }

  //Run the gaussian classifier algorithm
  m_ClassifierPtr->ClassifyImage();

  SetLabelledImage( m_ClassifierPtr->GetClassifiedImage() );

  ApplyMRFImageFilter();
  //Set the output labelled and allocate the memory
  LabelledImageType outputPtr = this->GetOutput();

  //Allocate the output buffer memory 
  outputPtr->SetBufferedRegion( outputPtr->GetRequestedRegion() );
  outputPtr->Allocate();

  //--------------------------------------------------------------------
  //Copy labelling result to the output buffer
  //--------------------------------------------------------------------
  // Set the iterators to the processed image
  //--------------------------------------------------------------------
  LabelledImageIterator  
  labelledImageIt( m_LabelledImage, m_LabelledImage->GetBufferedRegion() );

  //--------------------------------------------------------------------
  // Set the iterators to the output image buffer
  //--------------------------------------------------------------------
  LabelledImageIterator  
    outImageIt( outputPtr, outputPtr->GetBufferedRegion() );

  //--------------------------------------------------------------------

  while ( !outImageIt.IsAtEnd() )
    {
    LabelledImagePixelType labelvalue = 
      ( LabelledImagePixelType ) labelledImageIt.Get();

    outImageIt.Set( labelvalue );
    ++labelledImageIt;
    ++outImageIt;
    }// end while
        
}// end GenerateData

template<class TInputImage, class TClassifiedImage>
void 
GibbsPriorFilter<TInputImage, TClassifiedImage>
::ApplyMRFImageFilter()
{

  int maxNumPixelError =  
    static_cast<int>(m_ErrorTolerance * m_ImageWidth * m_ImageHeight * m_ImageDepth);

  int size = m_ImageWidth * m_ImageHeight * m_ImageDepth;
  int rowsize = m_ImageWidth;

  int numIter = 0;
  do
    {
    m_ErrorCounter = 0;
    MinimizeFunctional(); // minimize f_1 and f_3;
    numIter += 1;
    } 
  while(( numIter < m_MaximumNumberOfIterations ) && ( m_ErrorCounter >maxNumPixelError ) ); 

  RegionEraser();

  m_ErrorCounter = 0;
  srand ((unsigned)time(NULL));   
  for (int i = 0; i < size; i++ ) 
    {
    int randomPixel = (int) size*rand()/32768;
  
    if ((randomPixel > (rowsize - 1)) && (randomPixel < (size - rowsize)) 
        && (randomPixel%rowsize != 0) && (randomPixel%rowsize != rowsize-1)) 
      {
      GibbsTotalEnergy(randomPixel); // minimized f_2;
      }
    }

// erase noise regions in the image
//  RegionEraser();
}// ApplyMRFImageFilter

template<class TInputImage, class TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::MinimizeFunctional()
{
  //This implementation uses the ICM algorithm
  ApplyGibbsLabeller();
}

template<class TInputImage, class TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::ApplyGibbsLabeller()
{
  //--------------------------------------------------------------------
  // Set the iterators and the pixel type definition for the input image
  //-------------------------------------------------------------------
  InputImageIterator  inputImageIt(m_InputImage, 
                                   m_InputImage->GetBufferedRegion() );

  //--------------------------------------------------------------------
  // Set the iterators and the pixel type definition for the classified image
  //--------------------------------------------------------------------
  LabelledImageIterator  
    labelledImageIt(m_LabelledImage, m_LabelledImage->GetBufferedRegion());

  //Varible to store the origin pixel vector value
  InputImageVectorType OriginPixelVec;

  //Varible to store the origin pixel vector value
  InputImageVectorType ChangedPixelVec;

  //Variable to store the labelled pixel vector
  LabelledImagePixelType labelledPixel;

  //Variable to store the output pixel vector label after
  //the MRF classification
  LabelledImagePixelType outLabelledPix;

  //Set a variable to store the offset index
  LabelledImageIndexType offsetIndex3D = { 0, 0, 0};

  int size = m_ImageWidth * m_ImageHeight * m_ImageDepth;
  int frame = m_ImageWidth * m_ImageHeight;
  int rowsize = m_ImageWidth;

//  srand ((unsigned)time(NULL));   
//  for (int i = 0; i < size; i++ ) {
//    int randomPixel = (int) size*rand()/32768;

  int i = 0;
  double * dist = new double[m_NumberOfClasses];
  while ( !inputImageIt.IsAtEnd() )
    {
    offsetIndex3D[2] = i / frame;
    offsetIndex3D[1] = (i % frame) / m_ImageHeight;
    offsetIndex3D[0] = (i % frame) % m_ImageHeight;

    if ((i > (rowsize - 1)) && (i < (size - rowsize)) 
        && (i%rowsize != 0) && (i%rowsize != rowsize-1)) {
    OriginPixelVec = inputImageIt.Get();
    ChangedPixelVec[0] = GreyScalarBoundary(offsetIndex3D);
    if (OriginPixelVec[0] != ChangedPixelVec[0]) {
    inputImageIt.Set(ChangedPixelVec);
    //    m_ErrorCounter++;
    }

    m_ClassifierPtr->GetPixelDistance( ChangedPixelVec, dist );
    double minDist = 1e+20;
    int pixLabel = -1;
    
    for( int index = 0; index < m_NumberOfClasses; index++ )
      {
      if ( dist[index] < minDist )
        {
        minDist = dist[index];
        pixLabel = index;
        }// if
      }// for
    /*
      if ( dist[2] < (double)(m_BoundaryGradient) ) { 
      labelledImageIt.Set( 2 );
      } else labelledImageIt.Set( 1 );
    */
    labelledPixel = 
      ( LabelledImagePixelType ) labelledImageIt.Get();
    
    //Check if the label has changed then set the change flag in all the 
    //neighborhood of the current pixel
    if( pixLabel != ( int ) labelledPixel )
      {
      outLabelledPix = pixLabel;
      labelledImageIt.Set( outLabelledPix );
      m_ErrorCounter++;
      }   
    //    } 
    
    }
    i++;
    ++labelledImageIt;
    ++inputImageIt;
  }

  delete [] dist;

}//ApplyGibbslabeller

template<class TInputImage, class TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::RegionEraser()
{
  int i, j, size = m_ImageWidth * m_ImageHeight * m_ImageDepth;
  m_Region = (unsigned short*) malloc(sizeof(unsigned short)*size);
  m_RegionCount = (unsigned short*) malloc(sizeof(unsigned short)*size);

  LabelledImageIndexType offsetIndex3D = { 0, 0, 0};

  int frame = m_ImageWidth * m_ImageHeight;

  LabelledImageIterator  
    labelledImageIt(m_LabelledImage, m_LabelledImage->GetBufferedRegion());

  for ( i=0; i<size; i++ )
    {
    m_Region[i] = 0;
    m_RegionCount[i] = 1;
    }

  i = 0;
  int l = 0;
  int label;
  while ( !labelledImageIt.IsAtEnd() )
    {
    label = labelledImageIt.Get();
    if (( m_Region[i] == 0 ) && ((label != m_ObjectLabel) /*||
                                                            (labelledImageIt.Get() == 2)*/)) {
    //      if (labelledImageIt.Get() == 1) {
    if (LabelRegion(i, ++l, label) < m_ClusterSize) {
    for (j = 0; j < size; j++) {
    offsetIndex3D[2] = j / frame;
    offsetIndex3D[1] = (j % frame) / m_ImageHeight;
    offsetIndex3D[0] = (j % frame) % m_ImageHeight;
    if (m_Region[j] == l) {
    m_LabelledImage->SetPixel(offsetIndex3D, m_ObjectLabel);
    m_Region[j] = 0;
    }
    }
    l--;
    }
    /*} else {
      if (LabelRegion(i, ++l, 2) < m_ClusterSize) {
      for (j = 0; j < size; j++) {
      offsetIndex3D[2] = j / frame;
      offsetIndex3D[1] = (j % frame) / m_ImageHeight;
      offsetIndex3D[0] = (j % frame) % m_ImageHeight;
      if (m_Region[j] == l) {
      m_LabelledImage->SetPixel(offsetIndex3D, 1);
      m_Region[j] = 0;
      }
      }
      l--;
      }
      }*/
    }
    i++;
    ++labelledImageIt;
  }
}

template<class TInputImage, class TClassifiedImage>
int
GibbsPriorFilter<TInputImage, TClassifiedImage>
::LabelRegion(int i, int l, int change)
{
  int count = 1, m;
  int size = m_ImageWidth * m_ImageHeight * m_ImageDepth;
  int frame = m_ImageWidth * m_ImageHeight;
  int rowsize = m_ImageWidth;

  LabelledImageIndexType offsetIndex3D = { 0, 0, 0};

  m_Region[i] = l;

  offsetIndex3D[2] = i / frame;
  offsetIndex3D[1] = (i % frame) / m_ImageHeight;
  offsetIndex3D[0] = (i % frame) % m_ImageHeight;
  
  
  if (i-1 > -1) {
    offsetIndex3D[0]--;
  m = m_LabelledImage->GetPixel(offsetIndex3D);
    if ((m==change)&&(m_Region[i-1]==0))
    count += LabelRegion(i-1, l, change);
  offsetIndex3D[0]++;
  }

  if (i+1 < size) {
    offsetIndex3D[0]++;
  m = m_LabelledImage->GetPixel(offsetIndex3D);
    if ((m==change)&&(m_Region[i+1]==0))
    count += LabelRegion(i+1, l, change);
  offsetIndex3D[0]--;
  }

  if (i > rowsize - 1) {
    offsetIndex3D[1]--;
  m = m_LabelledImage->GetPixel(offsetIndex3D);
    if ((m==change)&&(m_Region[i-rowsize]==0))
    count += LabelRegion(i-rowsize, l, change);
  offsetIndex3D[1]++;
  }

  if (i < size - rowsize) {
    offsetIndex3D[1]++;
  m = m_LabelledImage->GetPixel(offsetIndex3D);
    if ((m==change)&&(m_Region[i+rowsize]==0))
    count += LabelRegion(i+rowsize, l, change);
  offsetIndex3D[1]--;
  }

  return count;
      
}

template<class TInputImage, class TClassifiedImage>
void
GibbsPriorFilter<TInputImage, TClassifiedImage>
::PrintSelf( std::ostream& os, Indent indent ) const
{
  Superclass::PrintSelf(os,indent);
  os << indent << "BoundaryGradient: "
     << m_BoundaryGradient << std::endl;
  os << indent << "GibbsNeighborhoodThreshold: "
     << m_GibbsNeighborhoodThreshold << std::endl;
  os << indent << "BoundaryWeight: "
     << m_BoundaryWeight << std::endl;
  os << indent << "MaximumNumberOfIterations: "
     << m_MaximumNumberOfIterations << std::endl;
  os << indent << "ErrorTolerance: "
     << m_ErrorTolerance << std::endl;
  os << indent << "ObjectLabel: "
     << m_ObjectLabel << std::endl;
  os << indent << "ClusterSize: "
     << m_ClusterSize << std::endl;
  os << indent << "NumberOfClasses: "
     << m_NumberOfClasses << std::endl;

}

} // end namespace itk

#endif
