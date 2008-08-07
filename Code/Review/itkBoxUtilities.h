/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    itkBoxUtilities.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef __itkBoxUtilities_h
#define __itkBoxUtilities_h
#include "itkProgressReporter.h"
#include "itkShapedNeighborhoodIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkConstantBoundaryCondition.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkOffset.h"
#include "itkNumericTraits.h"
#include "itkNeighborhoodAlgorithm.h"
#include "itkShapedNeighborhoodIterator.h"
#include "itkZeroFluxNeumannBoundaryCondition.h"

namespace itk {

template< class TIterator >
TIterator*
setConnectivityEarlyBox( TIterator* it, bool fullyConnected=false )
{
  // activate the "previous" neighbours
  typename TIterator::OffsetType offset;
  it->ClearActiveList();
  if( !fullyConnected) 
    {
    // only activate the neighbors that are face connected
    // to the current pixel. do not include the center pixel
    offset.Fill( 0 );
    for( unsigned int d=0; d < TIterator::Dimension; ++d )
      {
      offset[d] = -1;
      it->ActivateOffset( offset );
      offset[d] = 0;
      }
    }
  else
    {
    // activate all neighbors that are face+edge+vertex
    // connected to the current pixel. do not include the center pixel
    unsigned int centerIndex = it->GetCenterNeighborhoodIndex();
    for( unsigned int d=0; d < centerIndex; d++ )
      {
      offset = it->GetOffset( d );
      // check for positives in any dimension
      bool keep = true;
      for (unsigned i = 0; i < TIterator::Dimension; i++)
        {
        if (offset[i] > 0)
          {
          keep = false;
          break;
          }
        }
      if (keep)
        {
        it->ActivateOffset( offset );
        }
      }
    offset.Fill(0);
    it->DeactivateOffset( offset );
    }
  return it;
}

template <class TInputImage, class TOutputImage>
void
BoxAccumulateFunction(typename TInputImage::ConstPointer inputImage, 
                      typename TOutputImage::Pointer outputImage, 
                      typename TInputImage::RegionType inputRegion,
                      typename TOutputImage::RegionType outputRegion,
                      ProgressReporter &progress)
{
  // typedefs
  typedef TInputImage                              InputImageType;
  typedef typename TInputImage::RegionType         RegionType;
  typedef typename TInputImage::SizeType           SizeType;
  typedef typename TInputImage::IndexType          IndexType;
  typedef typename TInputImage::PixelType          PixelType;
  typedef typename TInputImage::OffsetType         OffsetType;
  typedef TOutputImage                             OutputImageType;
  typedef typename TOutputImage::PixelType         OutputPixelType;
  typedef typename TInputImage::PixelType          InputPixelType;
  
  typedef ImageRegionConstIterator<TInputImage>    InputIterator;
  typedef ImageRegionIterator<TOutputImage>        OutputIterator;

  typedef ShapedNeighborhoodIterator<TOutputImage> NOutputIterator;
  InputIterator inIt( inputImage, inputRegion);
  typename TInputImage::SizeType kernelRadius;
  kernelRadius.Fill(1);

  NOutputIterator noutIt( kernelRadius, outputImage, outputRegion);
  // this iterator is fully connected
  setConnectivityEarlyBox( &noutIt, true );

  ConstantBoundaryCondition<OutputImageType> oBC;
  oBC.SetConstant( NumericTraits< OutputPixelType >::Zero );
  noutIt.OverrideBoundaryCondition(&oBC);
  // This uses several iterators. An alternative and probably better
  // approach would be to copy the input to the output and convolve
  // with the following weights (in 2D)
  //   -(dim - 1)  1
  //       1       1
  // The result of each convolution needs to get written back to the
  // image being convolved so that the accumulation propogates
  // This should be implementable with neighborhood operators.

  std::vector<int> Weights;
  typename NOutputIterator::ConstIterator sIt;
  for( typename NOutputIterator::IndexListType::const_iterator idxIt = noutIt.GetActiveIndexList().begin();
       idxIt != noutIt.GetActiveIndexList().end();
       idxIt++ )
    {
    OffsetType offset = noutIt.GetOffset(*idxIt);
    int w = -1;
    for (unsigned k = 0; k < InputImageType::ImageDimension; k++)
      {
      if (offset[k] != 0)
        w *= offset[k];
      }
//     std::cout << offset << "  " << w << std::endl;
    Weights.push_back(w);
    }

  for (inIt.GoToBegin(), noutIt.GoToBegin(); !noutIt.IsAtEnd(); ++inIt, ++noutIt )
    {
    OutputPixelType Sum = 0;
    int k;
    for (k = 0, sIt = noutIt.Begin(); !sIt.IsAtEnd();++sIt, ++k)
      {
      Sum += sIt.Get() * Weights[k];
      }
    noutIt.SetCenterPixel(Sum + inIt.Get());
    progress.CompletedPixel();
    }
}

// a function to generate corners of arbitary dimension box
template <class ImType>
std::vector<typename ImType::OffsetType> CornerOffsets(typename ImType::ConstPointer im)
{
  typedef typename itk::ShapedNeighborhoodIterator<ImType> NIterator;
  typename ImType::SizeType unitradius;
  unitradius.Fill(1);
  NIterator N1(unitradius, im, im->GetRequestedRegion());
  unsigned int centerIndex = N1.GetCenterNeighborhoodIndex();
  typename NIterator::OffsetType offset;
  std::vector<typename ImType::OffsetType> result;
  for( unsigned int d=0; d < centerIndex*2 + 1; d++ )
    {
    offset = N1.GetOffset( d );
    // check whether this is a corner - corners have no zeros
    bool corner = true;
    for (unsigned k = 0; k < ImType::ImageDimension; k++)
      {
      if (offset[k] == 0)
        {
        corner = false;
        break;
        }
      }
    if (corner)
      result.push_back(offset);
    }
  return(result);
}

template <class TInputImage, class TOutputImage>
void
BoxMeanCalculatorFunction(typename TInputImage::ConstPointer accImage, 
                          typename TOutputImage::Pointer outputImage, 
                          typename TInputImage::RegionType inputRegion,
                          typename TOutputImage::RegionType outputRegion,
                          typename TInputImage::SizeType Radius,
                          ProgressReporter &progress)
{
  // typedefs
  typedef TInputImage                                         InputImageType;
  typedef typename TInputImage::RegionType                    RegionType;
  typedef typename TInputImage::SizeType                      SizeType;
  typedef typename TInputImage::IndexType                     IndexType;
  typedef typename TInputImage::PixelType                     PixelType;
  typedef typename TInputImage::OffsetType                    OffsetType;
  typedef TOutputImage                                        OutputImageType;
  typedef typename TOutputImage::PixelType                    OutputPixelType;
  typedef typename TInputImage::PixelType                     InputPixelType;
   // use the face generator for speed
  typedef typename itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType>
                                                              FaceCalculatorType;
  typedef typename FaceCalculatorType::FaceListType           FaceListType;
  typedef typename FaceCalculatorType::FaceListType::iterator FaceListTypeIt;
  FaceCalculatorType faceCalculator;

  FaceListType faceList;
  FaceListTypeIt fit;
  ZeroFluxNeumannBoundaryCondition<TInputImage> nbc;

  // this process is actually slightly asymmetric because we need to
  // subtract rectangles that are next to our kernel, not overlapping it
  SizeType kernelSize, internalRadius, RegionLimit;
  IndexType RegionStart = inputRegion.GetIndex();
  for( int i=0; i<TInputImage::ImageDimension; i++ )
    {
    kernelSize[i] = Radius[i] * 2 + 1;
    internalRadius[i] = Radius[i] + 1;
    RegionLimit[i] = inputRegion.GetSize()[i] + RegionStart[i] - 1;
    }

  typedef typename itk::NumericTraits<OutputPixelType>::RealType AccPixType;
  // get a set of offsets to corners for a unit hypercube in this image
  std::vector<OffsetType> UnitCorners = CornerOffsets<TInputImage>(accImage);
  std::vector<OffsetType> RealCorners;
  std::vector<AccPixType> Weights;
  // now compute the weights
  for (unsigned k = 0; k < UnitCorners.size(); k++)
    {
    int prod = 1;
    OffsetType ThisCorner;
    for (unsigned i = 0; i < TInputImage::ImageDimension; i++)
      {
      prod *= UnitCorners[k][i];
      if (UnitCorners[k][i] > 0)
        {
        ThisCorner[i] = Radius[i];
        }
      else
        {
        ThisCorner[i] = -(Radius[i]+1);
        }
      }
    Weights.push_back((AccPixType)prod);
    RealCorners.push_back(ThisCorner);
    }


  faceList = faceCalculator(accImage, outputRegion, internalRadius);
  // start with the body region
  for (fit = faceList.begin(); fit != faceList.end(); ++fit)
    {
    if (fit == faceList.begin())
      {
      // this is the body region. This is meant to be an optimized
      // version that doesn't use neigborhood regions
      // compute the various offsets
      AccPixType pixelscount = 1;
      for (unsigned i = 0; i < TInputImage::ImageDimension; i++)
        {
        pixelscount *= (AccPixType)(2*Radius[i] + 1);
        }
      
      typedef typename itk::ImageRegionIterator<OutputImageType> OutputIteratorType;
      typedef typename itk::ImageRegionConstIterator<InputImageType> InputIteratorType;

      typedef std::vector<InputIteratorType> CornerItVecType;
      CornerItVecType CornerItVec;
      // set up the iterators for each corner
      for (unsigned k = 0; k < RealCorners.size(); k++)
        {
        typename InputImageType::RegionType tReg=(*fit);
        tReg.SetIndex(tReg.GetIndex() + RealCorners[k]);
        InputIteratorType tempIt(accImage, tReg);
        tempIt.GoToBegin();
        CornerItVec.push_back(tempIt);
        }
      // set up the output iterator
      OutputIteratorType oIt(outputImage, *fit);
      // now do the work
      for (oIt.GoToBegin(); !oIt.IsAtEnd(); ++oIt)
        {
        AccPixType Sum = 0;
        // check each corner
        for (unsigned k = 0; k < CornerItVec.size(); k++)
          {
          Sum += Weights[k] * CornerItVec[k].Get();
          // increment each corner iterator
          ++(CornerItVec[k]);
          }
        oIt.Set(static_cast<OutputPixelType>(Sum/pixelscount));
        progress.CompletedPixel();
        }
      }
    else
      {
      // now we need to deal with the border regions
      typedef typename itk::ImageRegionIteratorWithIndex<OutputImageType> OutputIteratorType;
      OutputIteratorType oIt(outputImage, *fit);
      // now do the work
      for (oIt.GoToBegin(); !oIt.IsAtEnd(); ++oIt)
        {
        // figure out the number of pixels in the box by creating an
        // equivalent region and cropping - this could probably be
        // included in the loop below.
        RegionType currentKernelRegion;
        currentKernelRegion.SetSize( kernelSize );
        // compute the region's index
        IndexType kernelRegionIdx = oIt.GetIndex();
        IndexType CentIndex = kernelRegionIdx;
        for( int i=0; i<TInputImage::ImageDimension; i++ )
          {
          kernelRegionIdx[i] -= Radius[i];
          }
        currentKernelRegion.SetIndex( kernelRegionIdx );
        currentKernelRegion.Crop( inputRegion );
        long edgepixelscount = currentKernelRegion.GetNumberOfPixels();
        AccPixType Sum = 0;
        // rules are : for each corner,
        //               for each dimension
        //                  if dimension offset is positive -> this is
        //                  a leading edge. Crop if outside the input
        //                  region 
        //                  if dimension offset is negative -> this is
        //                  a trailing edge. Ignore if it is outside
        //                  image region
        for (unsigned k = 0; k < RealCorners.size(); k++)
          {
          IndexType ThisCorner = CentIndex + RealCorners[k];
          bool IncludeCorner = true;
          for (unsigned j = 0; j < TInputImage::ImageDimension; j++)
            {
            if (UnitCorners[k][j] > 0)
              {
              // leading edge - crop it
              ThisCorner[j] = std::min(ThisCorner[j], (long)RegionLimit[j]);
              }
            else
              {
              // trailing edge - check bounds
              if (ThisCorner[j] < RegionStart[j])
                {
                IncludeCorner = false;
                break;
                }
              }
            }
          if (IncludeCorner)
            {
            Sum += accImage->GetPixel(ThisCorner) * Weights[k];
            }
          }

        oIt.Set(static_cast<OutputPixelType>(Sum/(AccPixType)edgepixelscount));
        progress.CompletedPixel();
        }
      }
    }
}


template <class TInputImage, class TOutputImage>
void
BoxSigmaCalculatorFunction(typename TInputImage::ConstPointer accImage, 
                          typename TOutputImage::Pointer outputImage, 
                          typename TInputImage::RegionType inputRegion,
                          typename TOutputImage::RegionType outputRegion,
                          typename TInputImage::SizeType Radius,
                          ProgressReporter &progress)
{
  // typedefs
  typedef TInputImage                                         InputImageType;
  typedef typename TInputImage::RegionType                    RegionType;
  typedef typename TInputImage::SizeType                      SizeType;
  typedef typename TInputImage::IndexType                     IndexType;
  typedef typename TInputImage::PixelType                     PixelType;
  typedef typename TInputImage::OffsetType                    OffsetType;
  typedef TOutputImage                                        OutputImageType;
  typedef typename TOutputImage::PixelType                    OutputPixelType;
  typedef typename TInputImage::PixelType                     InputPixelType;
  typedef typename InputPixelType::ValueType                  ValueType;
   // use the face generator for speed
  typedef typename itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType>
                                                              FaceCalculatorType;
  typedef typename FaceCalculatorType::FaceListType           FaceListType;
  typedef typename FaceCalculatorType::FaceListType::iterator FaceListTypeIt;
  FaceCalculatorType faceCalculator;

  FaceListType faceList;
  FaceListTypeIt fit;
  ZeroFluxNeumannBoundaryCondition<TInputImage> nbc;

  // this process is actually slightly asymmetric because we need to
  // subtract rectangles that are next to our kernel, not overlapping it
  SizeType kernelSize, internalRadius, RegionLimit;
  IndexType RegionStart = inputRegion.GetIndex();
  for( int i=0; i<TInputImage::ImageDimension; i++ )
    {
    kernelSize[i] = Radius[i] * 2 + 1;
    internalRadius[i] = Radius[i] + 1;
    RegionLimit[i] = inputRegion.GetSize()[i] + RegionStart[i] - 1;
    }

  typedef typename itk::NumericTraits<OutputPixelType>::RealType AccPixType;
  // get a set of offsets to corners for a unit hypercube in this image
  std::vector<OffsetType> UnitCorners = CornerOffsets<TInputImage>(accImage);
  std::vector<OffsetType> RealCorners;
  std::vector<AccPixType> Weights;
  // now compute the weights
  for (unsigned k = 0; k < UnitCorners.size(); k++)
    {
    int prod = 1;
    OffsetType ThisCorner;
    for (unsigned i = 0; i < TInputImage::ImageDimension; i++)
      {
      prod *= UnitCorners[k][i];
      if (UnitCorners[k][i] > 0)
        {
        ThisCorner[i] = Radius[i];
        }
      else
        {
        ThisCorner[i] = -(Radius[i]+1);
        }
      }
    Weights.push_back((AccPixType)prod);
    RealCorners.push_back(ThisCorner);
    }


  faceList = faceCalculator(accImage, outputRegion, internalRadius);
  // start with the body region
  for (fit = faceList.begin(); fit != faceList.end(); ++fit)
    {
    if (fit == faceList.begin())
      {
      // this is the body region. This is meant to be an optimized
      // version that doesn't use neigborhood regions
      // compute the various offsets
      AccPixType pixelscount = 1;
      for (unsigned i = 0; i < TInputImage::ImageDimension; i++)
        {
        pixelscount *= (AccPixType)(2*Radius[i] + 1);
        }
      
      typedef typename itk::ImageRegionIterator<OutputImageType> OutputIteratorType;
      typedef typename itk::ImageRegionConstIterator<InputImageType> InputIteratorType;

      typedef std::vector<InputIteratorType> CornerItVecType;
      CornerItVecType CornerItVec;
      // set up the iterators for each corner
      for (unsigned k = 0; k < RealCorners.size(); k++)
        {
        typename InputImageType::RegionType tReg=(*fit);
        tReg.SetIndex(tReg.GetIndex() + RealCorners[k]);
        InputIteratorType tempIt(accImage, tReg);
        tempIt.GoToBegin();
        CornerItVec.push_back(tempIt);
        }
      // set up the output iterator
      OutputIteratorType oIt(outputImage, *fit);
      // now do the work
      for (oIt.GoToBegin(); !oIt.IsAtEnd(); ++oIt)
        {
        AccPixType Sum = 0;
        AccPixType SquareSum = 0;
        // check each corner
        for (unsigned k = 0; k < CornerItVec.size(); k++)
          {
          const InputPixelType & i = CornerItVec[k].Get();
          Sum += Weights[k] * i[0];
          SquareSum += Weights[k] * i[1];
          // increment each corner iterator
          ++(CornerItVec[k]);
          }

        oIt.Set(static_cast<OutputPixelType>( vcl_sqrt( ( SquareSum - Sum*Sum/pixelscount ) / ( pixelscount -1 ) ) ) );
        progress.CompletedPixel();
        }
      }
    else
      {
      // now we need to deal with the border regions
      typedef typename itk::ImageRegionIteratorWithIndex<OutputImageType> OutputIteratorType;
      OutputIteratorType oIt(outputImage, *fit);
      // now do the work
      for (oIt.GoToBegin(); !oIt.IsAtEnd(); ++oIt)
        {
        // figure out the number of pixels in the box by creating an
        // equivalent region and cropping - this could probably be
        // included in the loop below.
        RegionType currentKernelRegion;
        currentKernelRegion.SetSize( kernelSize );
        // compute the region's index
        IndexType kernelRegionIdx = oIt.GetIndex();
        IndexType CentIndex = kernelRegionIdx;
        for( int i=0; i<TInputImage::ImageDimension; i++ )
          {
          kernelRegionIdx[i] -= Radius[i];
          }
        currentKernelRegion.SetIndex( kernelRegionIdx );
        currentKernelRegion.Crop( inputRegion );
        long edgepixelscount = currentKernelRegion.GetNumberOfPixels();
        AccPixType Sum = 0;
        AccPixType SquareSum = 0;
        // rules are : for each corner,
        //               for each dimension
        //                  if dimension offset is positive -> this is
        //                  a leading edge. Crop if outside the input
        //                  region 
        //                  if dimension offset is negative -> this is
        //                  a trailing edge. Ignore if it is outside
        //                  image region
        for (unsigned k = 0; k < RealCorners.size(); k++)
          {
          IndexType ThisCorner = CentIndex + RealCorners[k];
          bool IncludeCorner = true;
          for (unsigned j = 0; j < TInputImage::ImageDimension; j++)
            {
            if (UnitCorners[k][j] > 0)
              {
              // leading edge - crop it
              ThisCorner[j] = std::min(ThisCorner[j], (long)RegionLimit[j]);
              }
            else
              {
              // trailing edge - check bounds
              if (ThisCorner[j] < RegionStart[j])
                {
                IncludeCorner = false;
                break;
                }
              }
            }
          if (IncludeCorner)
            {
            const InputPixelType & i = accImage->GetPixel(ThisCorner);
            Sum += Weights[k] * i[0];
            SquareSum += Weights[k] * i[1];
            }
          }

        oIt.Set(static_cast<OutputPixelType>( vcl_sqrt( ( SquareSum - Sum*Sum/edgepixelscount ) / ( edgepixelscount -1 ) ) ) );
        progress.CompletedPixel();
        }
      }
    }
}


template <class TInputImage, class TOutputImage>
void
BoxSquareAccumulateFunction(typename TInputImage::ConstPointer inputImage, 
                      typename TOutputImage::Pointer outputImage, 
                      typename TInputImage::RegionType inputRegion,
                      typename TOutputImage::RegionType outputRegion,
                      ProgressReporter &progress)
{
  // typedefs
  typedef TInputImage                              InputImageType;
  typedef typename TInputImage::RegionType         RegionType;
  typedef typename TInputImage::SizeType           SizeType;
  typedef typename TInputImage::IndexType          IndexType;
  typedef typename TInputImage::PixelType          PixelType;
  typedef typename TInputImage::OffsetType         OffsetType;
  typedef TOutputImage                             OutputImageType;
  typedef typename TOutputImage::PixelType         OutputPixelType;
  typedef typename OutputPixelType::ValueType      ValueType;
  typedef typename TInputImage::PixelType          InputPixelType;
  
  typedef ImageRegionConstIterator<TInputImage>    InputIterator;
  typedef ImageRegionIterator<TOutputImage>        OutputIterator;

  typedef ShapedNeighborhoodIterator<TOutputImage> NOutputIterator;
  InputIterator inIt( inputImage, inputRegion);
  typename TInputImage::SizeType kernelRadius;
  kernelRadius.Fill(1);

  NOutputIterator noutIt( kernelRadius, outputImage, outputRegion);
  // this iterator is fully connected
  setConnectivityEarlyBox( &noutIt, true );

  ConstantBoundaryCondition<OutputImageType> oBC;
  oBC.SetConstant( NumericTraits< OutputPixelType >::Zero );
  noutIt.OverrideBoundaryCondition(&oBC);
  // This uses several iterators. An alternative and probably better
  // approach would be to copy the input to the output and convolve
  // with the following weights (in 2D)
  //   -(dim - 1)  1
  //       1       1
  // The result of each convolution needs to get written back to the
  // image being convolved so that the accumulation propogates
  // This should be implementable with neighborhood operators.

  std::vector<int> Weights;
  typename NOutputIterator::ConstIterator sIt;
  for( typename NOutputIterator::IndexListType::const_iterator idxIt = noutIt.GetActiveIndexList().begin();
       idxIt != noutIt.GetActiveIndexList().end();
       idxIt++ )
    {
    OffsetType offset = noutIt.GetOffset(*idxIt);
    int w = -1;
    for (unsigned k = 0; k < InputImageType::ImageDimension; k++)
      {
      if (offset[k] != 0)
        w *= offset[k];
      }
    Weights.push_back(w);
    }

  for (inIt.GoToBegin(), noutIt.GoToBegin(); !noutIt.IsAtEnd(); ++inIt, ++noutIt )
    {
    ValueType Sum = 0;
    ValueType SquareSum = 0;
    int k;
    for (k = 0, sIt = noutIt.Begin(); !sIt.IsAtEnd();++sIt, ++k)
      {
      const OutputPixelType & v = sIt.Get();
      Sum += v[0] * Weights[k];
      SquareSum += v[1] * Weights[k];
      }
    OutputPixelType o;
    const InputPixelType & i = inIt.Get();
    o[0] = Sum + i;
    o[1] = SquareSum + i*i;
    noutIt.SetCenterPixel( o );
    progress.CompletedPixel();
    }
}


} //namespace itk

#endif
