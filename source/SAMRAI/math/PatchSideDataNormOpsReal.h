/*************************************************************************
 *
 * This file is part of the SAMRAI distribution.  For full copyright
 * information, see COPYRIGHT and LICENSE.
 *
 * Copyright:     (c) 1997-2022 Lawrence Livermore National Security, LLC
 * Description:   Templated norm operations for real side-centered data.
 *
 ************************************************************************/

#ifndef included_math_PatchSideDataNormOpsReal
#define included_math_PatchSideDataNormOpsReal

#include "SAMRAI/SAMRAI_config.h"

#include "SAMRAI/pdat/SideData.h"
#include "SAMRAI/math/ArrayDataNormOpsReal.h"
#include "SAMRAI/hier/Box.h"

#include <memory>

namespace SAMRAI {
namespace math {

/**
 * Class PatchSideDataNormOpsReal provides a collection of common
 * norm operations that may be applied to real (double or float)
 * numerical side-centered patch data.  The primary intent of this class is
 * to define part of the interface for an PatchSideDataOpsReal object
 * which provides access operations that may be used to manipulate
 * side-centered patch data.  Each member function accepts a box argument
 * indicating the region of index space on which the operation should be
 * performed.  The operation will be performed on the intersection of this
 * box and those boxes corresponding to the patch data objects.  Also, each
 * operation allows an additional side-centered patch data object to be used
 * to represent a control volume that weights the contribution of each data
 * entry in the given norm calculation.  Note that the control volume patch
 * data must be of type double and have side-centered geometry (i.e., the
 * same as the data itself).  The use of control volumes is important when
 * these operations are used in vector kernels where the data resides over
 * multiple levels of spatial resolution in an AMR hierarchy.  If the control
 * volume is not given in the function call, it will be ignored in the
 * calculation.  Also, note that the depth of the control volume patch data
 * object must be either 1 or be equal to the depth of the other data objects.
 *
 * These operations typically apply only to the numerical standard built-in
 * types, such as double, float, and the complex type (which may or may not
 * be a built-in type depending on the C++ compiler).  This templated
 * class should only be used to instantiate objects with double or float as
 * the template parameter.  Note that a similar set of norm operations is
 * implemented for complex patch data in the class
 * PatchSideDataNormOpsComplex.
 *
 * @see ArrayDataNormOpsReal
 */

template<class TYPE>
class PatchSideDataNormOpsReal
{
public:
   /**
    * Empty constructor and destructor.
    */
   PatchSideDataNormOpsReal();

   virtual ~PatchSideDataNormOpsReal<TYPE>();

   /**
    * Return the number of data values for the side-centered data object
    * in the given box.  Note that it is assumed that the box refers to
    * the cell-centered index space corresponding to the patch hierarchy.
    *
    * @pre data
    * @pre data->getDim() == box.getDim()
    */
   size_t
   numberOfEntries(
      const std::shared_ptr<pdat::SideData<TYPE> >& data,
      const hier::Box& box) const;

   /**
    * Return sum of control volume entries for the side-centered data object.
    *
    * @pre data && cvol
    * @pre data->getDirectionVector() == hier::IntVector::min(data->getDirectionVector(), cvol->getDirectionVector())
    */
   double
   sumControlVolumes(
      const std::shared_ptr<pdat::SideData<TYPE> >& data,
      const std::shared_ptr<pdat::SideData<double> >& cvol,
      const hier::Box& box) const;

   /**
    * Set destination component to absolute value of source component.
    * That is, each destination entry is set to \f$d_i = \| s_i \|\f$.
    *
    * @pre dst && src
    * @pre dst->getDirectionVector() == src->getDirectionVector()
    * @pre (dst->getDim() == src->getDim()) && (dst->getDim() == box.getDim())
    */
   void
   abs(
      const std::shared_ptr<pdat::SideData<TYPE> >& dst,
      const std::shared_ptr<pdat::SideData<TYPE> >& src,
      const hier::Box& box) const;

   /**
    * Return discrete \f$L_1\f$-norm of the data using the control volume to
    * weight the contribution of each data entry to the sum.  That is, the
    * return value is the sum \f$\sum_i ( \| data_i \| cvol_i )\f$.  If the
    * control volume is NULL, the return value is \f$\sum_i ( \| data_i \| )\f$.
    *
    * @pre data
    * @pre data->getDim() == box.getDim()
    * @pre !cvol || (data->getDim() == cvol->getDim())
    * @pre !cvol || (data->getDirectionVector() == hier::IntVector::min(data->getDirectionVector(), cvol->getDirectionVector()))
    */
   double
   L1Norm(
      const std::shared_ptr<pdat::SideData<TYPE> >& data,
      const hier::Box& box,
      const std::shared_ptr<pdat::SideData<double> >& cvol =
         std::shared_ptr<pdat::SideData<double> >()) const;

   /**
    * Return discrete \f$L_2\f$-norm of the data using the control volume to
    * weight the contribution of each data entry to the sum.  That is, the
    * return value is the sum \f$\sqrt{ \sum_i ( (data_i)^2 cvol_i ) }\f$.
    * If the control volume is NULL, the return value is
    * \f$\sqrt{ \sum_i ( (data_i)^2 cvol_i ) }\f$.
    *
    * @pre data
    * @pre data->getDim() == box.getDim()
    * @pre !cvol || (data->getDim() == cvol->getDim())
    * @pre !cvol || (data->getDirectionVector() == hier::IntVector::min(data->getDirectionVector(), cvol->getDirectionVector()))
    */
   double
   L2Norm(
      const std::shared_ptr<pdat::SideData<TYPE> >& data,
      const hier::Box& box,
      const std::shared_ptr<pdat::SideData<double> >& cvol =
         std::shared_ptr<pdat::SideData<double> >()) const;

   /**
    * Return discrete weighted \f$L_2\f$-norm of the data using the control
    * volume to weight the contribution of the data and weight entries to
    * the sum.  That is, the return value is the sum \f$\sqrt{ \sum_i (
    * (data_i * weight_i)^2 cvol_i ) }\f$.  If the control volume is NULL,
    * the return value is \f$\sqrt{ \sum_i ( (data_i * weight_i)^2 ) }\f$.
    *
    * @pre data && weight
    * @pre (data->getDim() == weight->getDim()) &&
    *      (data->getDim() == box.getDim())
    * @pre data->getDirectionVector() == hier::IntVector::min(data->getDirectionVector(), weight->getDirectionVector())
    * @pre !cvol || (data->getDim() == cvol->getDim())
    * @pre !cvol || (data->getDirectionVector() == hier::IntVector::min(data->getDirectionVector(), cvol->getDirectionVector()))
    */
   double
   weightedL2Norm(
      const std::shared_ptr<pdat::SideData<TYPE> >& data,
      const std::shared_ptr<pdat::SideData<TYPE> >& weight,
      const hier::Box& box,
      const std::shared_ptr<pdat::SideData<double> >& cvol =
         std::shared_ptr<pdat::SideData<double> >()) const;

   /**
    * Return discrete root mean squared norm of the data.  If the control
    * volume is not NULL, the return value is the \f$L_2\f$-norm divided by
    * the square root of the sum of the control volumes.  Otherwise, the
    * return value is the \f$L_2\f$-norm divided by the square root of the
    * number of data entries.
    *
    * @pre data
    * @pre !cvol || (data->getDim() == cvol->getDim())
    */
   double
   RMSNorm(
      const std::shared_ptr<pdat::SideData<TYPE> >& data,
      const hier::Box& box,
      const std::shared_ptr<pdat::SideData<double> >& cvol =
         std::shared_ptr<pdat::SideData<double> >()) const;

   /**
    * Return discrete weighted root mean squared norm of the data.  If the
    * control volume is not NULL, the return value is the weighted \f$L_2\f$-norm
    * divided by the square root of the sum of the control volumes.  Otherwise,
    * the return value is the weighted \f$L_2\f$-norm divided by the square root
    * of the number of data entries.
    *
    * @pre data && weight
    * @pre data->getDim() == box.getDim()
    * @pre !cvol || (data->getDim() == cvol->getDim())
    */
   double
   weightedRMSNorm(
      const std::shared_ptr<pdat::SideData<TYPE> >& data,
      const std::shared_ptr<pdat::SideData<TYPE> >& weight,
      const hier::Box& box,
      const std::shared_ptr<pdat::SideData<double> >& cvol =
         std::shared_ptr<pdat::SideData<double> >()) const;

   /**
    * Return the \f$\max\f$-norm of the data using the control volume to weight
    * the contribution of each data entry to the maximum.  That is, the return
    * value is \f$\max_i ( \| data_i \| )\f$, where the max is over the data
    * elements where \f$cvol_i > 0\f$.  If the control volume is NULL, it is
    * ignored during the computation of the maximum.
    *
    * @pre data && weight
    * @pre data->getDim() == box.getDim()
    * @pre !cvol || (data->getDim() == cvol->getDim())
    * @pre !cvol || (data->getDirectionVector() == hier::IntVector::min(data->getDirectionVector(), cvol->getDirectionVector()))
    */
   double
   maxNorm(
      const std::shared_ptr<pdat::SideData<TYPE> >& data,
      const hier::Box& box,
      const std::shared_ptr<pdat::SideData<double> >& cvol =
         std::shared_ptr<pdat::SideData<double> >()) const;

   /**
    * Return the dot product of the two data arrays using the control volume
    * to weight the contribution of each product to the sum.  That is, the
    * return value is the sum \f$\sum_i ( data1_i * data2_i * cvol_i )\f$.
    * If the control volume is NULL, it is ignored during the summation.
    *
    * @pre data1 && data2
    * @pre data1->getDirectionVector() == data2->getDirectionVector()
    * @pre !cvol || (data1->getDim() == cvol->getDim())
    * @pre !cvol || (data1->getDirectionVector() == hier::IntVector::min(data1->getDirectionVector(), cvol->getDirectionVector()))
    */
   TYPE
   dot(
      const std::shared_ptr<pdat::SideData<TYPE> >& data1,
      const std::shared_ptr<pdat::SideData<TYPE> >& data2,
      const hier::Box& box,
      const std::shared_ptr<pdat::SideData<double> >& cvol =
         std::shared_ptr<pdat::SideData<double> >()) const;

   /**
    * Return the integral of the function represented by the data array.
    * The return value is the sum \f$\sum_i ( data_i * vol_i )\f$.
    *
    * @pre data && vol
    * @pre (data->getDim() == vol->getDim()) &&
    *      (data->getDim() == box.getDim())
    * @pre data->getDirectionVector() == hier::IntVector::min(data->getDirectionVector(), vol->getDirectionVector())
    */
   TYPE
   integral(
      const std::shared_ptr<pdat::SideData<TYPE> >& data,
      const hier::Box& box,
      const std::shared_ptr<pdat::SideData<double> >& vol) const;

private:
   // The following are not implemented:
   PatchSideDataNormOpsReal(
      const PatchSideDataNormOpsReal&);
   PatchSideDataNormOpsReal&
   operator = (
      const PatchSideDataNormOpsReal&);

   ArrayDataNormOpsReal<TYPE> d_array_ops;
};

}
}

#include "SAMRAI/math/PatchSideDataNormOpsReal.cpp"

#endif
