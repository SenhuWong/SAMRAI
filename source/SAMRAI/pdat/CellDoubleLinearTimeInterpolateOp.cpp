/*************************************************************************
 *
 * This file is part of the SAMRAI distribution.  For full copyright
 * information, see COPYRIGHT and LICENSE.
 *
 * Copyright:     (c) 1997-2022 Lawrence Livermore National Security, LLC
 * Description:   Linear time interp operator for cell-centered double patch data.
 *
 ************************************************************************/
#include "SAMRAI/pdat/CellDoubleLinearTimeInterpolateOp.h"

#include "SAMRAI/pdat/CellData.h"
#include "SAMRAI/pdat/CellVariable.h"
#include "SAMRAI/hier/Box.h"
#include "SAMRAI/hier/Index.h"
#include "SAMRAI/tbox/Utilities.h"
#include "SAMRAI/tbox/MathUtilities.h"
#include "SAMRAI/tbox/NVTXUtilities.h"

#include <memory>

/*
 *************************************************************************
 *
 * External declarations for FORTRAN  routines.
 *
 *************************************************************************
 */
extern "C" {

#ifdef __INTEL_COMPILER
#pragma warning(disable : 1419)
#endif

// in lintimint1d.f:
void SAMRAI_F77_FUNC(lintimeintcelldoub1d, LINTIMEINTCELLDOUB1D)(const int&,
                                                                 const int&,
                                                                 const int&, const int&,
                                                                 const int&, const int&,
                                                                 const int&, const int&,
                                                                 const double&,
                                                                 const double*, const double*,
                                                                 double*);
// in lintimint2d.f:
void SAMRAI_F77_FUNC(lintimeintcelldoub2d, LINTIMEINTCELLDOUB2D)(const int&,
                                                                 const int&,
                                                                 const int&, const int&,
                                                                 const int&, const int&,
                                                                 const int&, const int&,
                                                                 const int&, const int&,
                                                                 const int&, const int&,
                                                                 const int&, const int&,
                                                                 const int&, const int&,
                                                                 const double&,
                                                                 const double*, const double*,
                                                                 double*);
// in lintimint3d.f:
void SAMRAI_F77_FUNC(lintimeintcelldoub3d, LINTIMEINTCELLDOUB3D)(const int&,
                                                                 const int&, const int&,
                                                                 const int&, const int&, const int&,
                                                                 const int&, const int&, const int&,
                                                                 const int&, const int&, const int&,
                                                                 const int&, const int&, const int&,
                                                                 const int&, const int&, const int&,
                                                                 const int&, const int&, const int&,
                                                                 const int&, const int&, const int&,
                                                                 const double&,
                                                                 const double*, const double*,
                                                                 double*);
}

namespace SAMRAI
{
namespace pdat
{

CellDoubleLinearTimeInterpolateOp::CellDoubleLinearTimeInterpolateOp() : hier::TimeInterpolateOperator()
{
}

CellDoubleLinearTimeInterpolateOp::~CellDoubleLinearTimeInterpolateOp()
{
}

void
CellDoubleLinearTimeInterpolateOp::timeInterpolate(
   hier::PatchData& dst_data,
   const hier::Box& where,
   const hier::BoxOverlap& overlap,
   const hier::PatchData& src_data_old,
   const hier::PatchData& src_data_new) const
{
   NULL_USE(overlap);
   const tbox::Dimension& dim(where.getDim());

   const CellData<double>* old_dat =
       CPP_CAST<const CellData<double>*>(&src_data_old);
   const CellData<double>* new_dat =
       CPP_CAST<const CellData<double>*>(&src_data_new);
   CellData<double>* dst_dat =
       CPP_CAST<CellData<double>*>(&dst_data);

   TBOX_ASSERT(old_dat != 0);
   TBOX_ASSERT(new_dat != 0);
   TBOX_ASSERT(dst_dat != 0);
   TBOX_ASSERT((where * old_dat->getGhostBox()).isSpatiallyEqual(where));
   TBOX_ASSERT((where * new_dat->getGhostBox()).isSpatiallyEqual(where));
   TBOX_ASSERT((where * dst_dat->getGhostBox()).isSpatiallyEqual(where));
   TBOX_ASSERT_OBJDIM_EQUALITY4(dst_data, where, src_data_old, src_data_new);

   const hier::Index& old_ilo = old_dat->getGhostBox().lower();
   const hier::Index& old_ihi = old_dat->getGhostBox().upper();
   const hier::Index& new_ilo = new_dat->getGhostBox().lower();
   const hier::Index& new_ihi = new_dat->getGhostBox().upper();

   const hier::Index& dst_ilo = dst_dat->getGhostBox().lower();
   const hier::Index& dst_ihi = dst_dat->getGhostBox().upper();

   const hier::Index& ifirst = where.lower();
   const hier::Index& ilast = where.upper();

   const double old_time = old_dat->getTime();
   const double new_time = new_dat->getTime();
   const double dst_time = dst_dat->getTime();

   TBOX_ASSERT((old_time < dst_time ||
                tbox::MathUtilities<double>::equalEps(old_time, dst_time)) &&
               (dst_time < new_time ||
                tbox::MathUtilities<double>::equalEps(dst_time, new_time)));

   double tfrac = dst_time - old_time;
   double denom = new_time - old_time;
   if (denom > tbox::MathUtilities<double>::getMin()) {
      tfrac /= denom;
   } else {
      tfrac = 0.0;
   }

   for (int d = 0; d < dst_dat->getDepth(); ++d) {
      if (dim == tbox::Dimension(1)) {
         SAMRAI_F77_FUNC(lintimeintcelldoub1d, LINTIMEINTCELLDOUB1D)
         (ifirst(0),
          ilast(0),
          old_ilo(0), old_ihi(0),
          new_ilo(0), new_ihi(0),
          dst_ilo(0), dst_ihi(0),
          tfrac,
          old_dat->getPointer(d),
          new_dat->getPointer(d),
          dst_dat->getPointer(d));
      } else if (dim == tbox::Dimension(2)) {
#if defined(HAVE_RAJA)
         auto old_array = old_dat->getConstView<2>(d);
         auto new_array = new_dat->getConstView<2>(d);
         auto dst_array = dst_dat->getView<2>(d);

         hier::parallel_for_all(where, [=] SAMRAI_HOST_DEVICE(int j /*fastest*/, int k) {
            const double oldfrac = 1.0 - tfrac;
            dst_array(j, k) = old_array(j, k) * oldfrac + new_array(j, k) * tfrac;
         });
#else
         SAMRAI_F77_FUNC(lintimeintcelldoub2d, LINTIMEINTCELLDOUB2D)
         (ifirst(0),
          ifirst(1), ilast(0), ilast(1),
          old_ilo(0), old_ilo(1), old_ihi(0), old_ihi(1),
          new_ilo(0), new_ilo(1), new_ihi(0), new_ihi(1),
          dst_ilo(0), dst_ilo(1), dst_ihi(0), dst_ihi(1),
          tfrac,
          old_dat->getPointer(d),
          new_dat->getPointer(d),
          dst_dat->getPointer(d));
#endif  // test for RAJA
      } else if (dim == tbox::Dimension(3)) {
#if defined(HAVE_RAJA)
         auto old_array = old_dat->getConstView<3>(d);
         auto new_array = new_dat->getConstView<3>(d);
         auto dst_array = dst_dat->getView<3>(d);

         hier::parallel_for_all(where, [=] SAMRAI_HOST_DEVICE(int i /*fastest*/, int j, int k) {
            const double oldfrac = 1.0 - tfrac;
            dst_array(i, j, k) = old_array(i, j, k) * oldfrac + new_array(i, j, k) * tfrac;
         });
#else
         SAMRAI_F77_FUNC(lintimeintcelldoub3d, LINTIMEINTCELLDOUB3D)
         (ifirst(0),
          ifirst(1), ifirst(2),
          ilast(0), ilast(1), ilast(2),
          old_ilo(0), old_ilo(1), old_ilo(2),
          old_ihi(0), old_ihi(1), old_ihi(2),
          new_ilo(0), new_ilo(1), new_ilo(2),
          new_ihi(0), new_ihi(1), new_ihi(2),
          dst_ilo(0), dst_ilo(1), dst_ilo(2),
          dst_ihi(0), dst_ihi(1), dst_ihi(2),
          tfrac,
          old_dat->getPointer(d),
          new_dat->getPointer(d),
          dst_dat->getPointer(d));
#endif
      } else {
         TBOX_ERROR(
             "CellDoubleLinearTimeInterpolateOp::TimeInterpolate dim > 3 not supported"
             << std::endl);
      }
   }
}

}  // namespace pdat
}  // namespace SAMRAI
