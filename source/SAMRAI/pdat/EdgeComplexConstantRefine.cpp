/*************************************************************************
 *
 * This file is part of the SAMRAI distribution.  For full copyright
 * information, see COPYRIGHT and LICENSE.
 *
 * Copyright:     (c) 1997-2022 Lawrence Livermore National Security, LLC
 * Description:   Constant refine operator for edge-centered complex data on
 *                a  mesh.
 *
 ************************************************************************/
#include "SAMRAI/pdat/EdgeComplexConstantRefine.h"

#include "SAMRAI/tbox/Utilities.h"
#include "SAMRAI/hier/BoxContainer.h"
#include "SAMRAI/hier/Index.h"
#include "SAMRAI/pdat/EdgeData.h"
#include "SAMRAI/pdat/EdgeVariable.h"

/*
 *************************************************************************
 *
 * External declarations for FORTRAN  routines.
 *
 *************************************************************************
 */
extern "C" {

#ifdef __INTEL_COMPILER
#pragma warning (disable:1419)
#endif

// in conrefine1d.f:
void SAMRAI_F77_FUNC(conrefedgecplx1d, CONREFEDGECPLX1D) (const int&, const int&,
   const int&, const int&,
   const int&, const int&,
   const int&, const int&,
   const int *,
   const dcomplex *, dcomplex *);
// in conrefine2d.f:
void SAMRAI_F77_FUNC(conrefedgecplx2d0, CONREFEDGECPLX2D0) (const int&, const int&,
   const int&, const int&,
   const int&, const int&, const int&, const int&,
   const int&, const int&, const int&, const int&,
   const int&, const int&, const int&, const int&,
   const int *,
   const dcomplex *, dcomplex *);
void SAMRAI_F77_FUNC(conrefedgecplx2d1, CONREFEDGECPLX2D1) (const int&, const int&,
   const int&, const int&,
   const int&, const int&, const int&, const int&,
   const int&, const int&, const int&, const int&,
   const int&, const int&, const int&, const int&,
   const int *,
   const dcomplex *, dcomplex *);
// in conrefine3d.f:
void SAMRAI_F77_FUNC(conrefedgecplx3d0, CONREFEDGECPLX3D0) (const int&, const int&,
   const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int *,
   const dcomplex *, dcomplex *);
void SAMRAI_F77_FUNC(conrefedgecplx3d1, CONREFEDGECPLX3D1) (const int&, const int&,
   const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int *,
   const dcomplex *, dcomplex *);
void SAMRAI_F77_FUNC(conrefedgecplx3d2, CONREFEDGECPLX3D2) (const int&, const int&,
   const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int&, const int&, const int&,
   const int *,
   const dcomplex *, dcomplex *);
}

namespace SAMRAI {
namespace pdat {

EdgeComplexConstantRefine::EdgeComplexConstantRefine():
   hier::RefineOperator("CONSTANT_REFINE")
{
}

EdgeComplexConstantRefine::~EdgeComplexConstantRefine()
{
}

int
EdgeComplexConstantRefine::getOperatorPriority() const
{
   return 0;
}

hier::IntVector
EdgeComplexConstantRefine::getStencilWidth(const tbox::Dimension& dim) const
{
   return hier::IntVector::getZero(dim);
}

void
EdgeComplexConstantRefine::refine(
   hier::Patch& fine,
   const hier::Patch& coarse,
   const int dst_component,
   const int src_component,
   const hier::BoxOverlap& fine_overlap,
   const hier::IntVector& ratio) const
{
   const tbox::Dimension& dim(fine.getDim());

   std::shared_ptr<EdgeData<dcomplex> > cdata(
      SAMRAI_SHARED_PTR_CAST<EdgeData<dcomplex>, hier::PatchData>(
         coarse.getPatchData(src_component)));
   std::shared_ptr<EdgeData<dcomplex> > fdata(
      SAMRAI_SHARED_PTR_CAST<EdgeData<dcomplex>, hier::PatchData>(
         fine.getPatchData(dst_component)));

   const EdgeOverlap* t_overlap = CPP_CAST<const EdgeOverlap *>(&fine_overlap);

   TBOX_ASSERT(t_overlap != 0);

   TBOX_ASSERT(cdata);
   TBOX_ASSERT(fdata);
   TBOX_ASSERT(cdata->getDepth() == fdata->getDepth());
   TBOX_ASSERT_OBJDIM_EQUALITY3(fine, coarse, ratio);

   const hier::Box& cgbox(cdata->getGhostBox());

   const hier::Index& cilo = cgbox.lower();
   const hier::Index& cihi = cgbox.upper();
   const hier::Index& filo = fdata->getGhostBox().lower();
   const hier::Index& fihi = fdata->getGhostBox().upper();

   for (int axis = 0; axis < dim.getValue(); ++axis) {
      const hier::BoxContainer& boxes = t_overlap->getDestinationBoxContainer(axis);

      for (hier::BoxContainer::const_iterator b = boxes.begin();
           b != boxes.end(); ++b) {

         hier::Box fine_box(*b);
         TBOX_ASSERT_DIM_OBJDIM_EQUALITY1(dim, fine_box);

         for (tbox::Dimension::dir_t i = 0; i < dim.getValue(); ++i) {
            if (i != axis) {
               fine_box.setUpper(i, fine_box.upper(i) - 1);
            }
         }

         const hier::Box coarse_box = hier::Box::coarsen(fine_box, ratio);
         const hier::Index& ifirstc = coarse_box.lower();
         const hier::Index& ilastc = coarse_box.upper();
         const hier::Index& ifirstf = fine_box.lower();
         const hier::Index& ilastf = fine_box.upper();

         for (int d = 0; d < fdata->getDepth(); ++d) {
            if (dim == tbox::Dimension(1)) {
               SAMRAI_F77_FUNC(conrefedgecplx1d, CONREFEDGECPLX1D) (
                  ifirstc(0), ilastc(0),
                  ifirstf(0), ilastf(0),
                  cilo(0), cihi(0),
                  filo(0), fihi(0),
                  &ratio[0],
                  cdata->getPointer(0, d),
                  fdata->getPointer(0, d));
            } else if (dim == tbox::Dimension(2)) {
               if (axis == 0) {
                  SAMRAI_F77_FUNC(conrefedgecplx2d0, CONREFEDGECPLX2D0) (
                     ifirstc(0), ifirstc(1), ilastc(0), ilastc(1),
                     ifirstf(0), ifirstf(1), ilastf(0), ilastf(1),
                     cilo(0), cilo(1), cihi(0), cihi(1),
                     filo(0), filo(1), fihi(0), fihi(1),
                     &ratio[0],
                     cdata->getPointer(0, d),
                     fdata->getPointer(0, d));
               } else if (axis == 1) {
                  SAMRAI_F77_FUNC(conrefedgecplx2d1, CONREFEDGECPLX2D1) (
                     ifirstc(0), ifirstc(1), ilastc(0), ilastc(1),
                     ifirstf(0), ifirstf(1), ilastf(0), ilastf(1),
                     cilo(0), cilo(1), cihi(0), cihi(1),
                     filo(0), filo(1), fihi(0), fihi(1),
                     &ratio[0],
                     cdata->getPointer(1, d),
                     fdata->getPointer(1, d));
               }
            } else if (dim == tbox::Dimension(3)) {
               if (axis == 0) {
                  SAMRAI_F77_FUNC(conrefedgecplx3d0, CONREFEDGECPLX3D0) (
                     ifirstc(0), ifirstc(1), ifirstc(2),
                     ilastc(0), ilastc(1), ilastc(2),
                     ifirstf(0), ifirstf(1), ifirstf(2),
                     ilastf(0), ilastf(1), ilastf(2),
                     cilo(0), cilo(1), cilo(2),
                     cihi(0), cihi(1), cihi(2),
                     filo(0), filo(1), filo(2),
                     fihi(0), fihi(1), fihi(2),
                     &ratio[0],
                     cdata->getPointer(0, d),
                     fdata->getPointer(0, d));
               } else if (axis == 1) {
                  SAMRAI_F77_FUNC(conrefedgecplx3d1, CONREFEDGECPLX3D1) (
                     ifirstc(0), ifirstc(1), ifirstc(2),
                     ilastc(0), ilastc(1), ilastc(2),
                     ifirstf(0), ifirstf(1), ifirstf(2),
                     ilastf(0), ilastf(1), ilastf(2),
                     cilo(0), cilo(1), cilo(2),
                     cihi(0), cihi(1), cihi(2),
                     filo(0), filo(1), filo(2),
                     fihi(0), fihi(1), fihi(2),
                     &ratio[0],
                     cdata->getPointer(1, d),
                     fdata->getPointer(1, d));
               } else if (axis == 2) {
                  SAMRAI_F77_FUNC(conrefedgecplx3d2, CONREFEDGECPLX3D2) (
                     ifirstc(0), ifirstc(1), ifirstc(2),
                     ilastc(0), ilastc(1), ilastc(2),
                     ifirstf(0), ifirstf(1), ifirstf(2),
                     ilastf(0), ilastf(1), ilastf(2),
                     cilo(0), cilo(1), cilo(2),
                     cihi(0), cihi(1), cihi(2),
                     filo(0), filo(1), filo(2),
                     fihi(0), fihi(1), fihi(2),
                     &ratio[0],
                     cdata->getPointer(2, d),
                     fdata->getPointer(2, d));
               }
            } else {
               TBOX_ERROR(
                  "EdgeComplexConstantRefine::refine dimension > 3 not supported"
                  << std::endl);
            }
         }
      }
   }
}

}
}
