/*************************************************************************
 *
 * This file is part of the SAMRAI distribution.  For full copyright
 * information, see COPYRIGHT and LICENSE.
 *
 * Copyright:     (c) 1997-2022 Lawrence Livermore National Security, LLC
 * Description:   Main program to test SAMRAI-KINSOL vector interface.
 *
 ************************************************************************/

#include "SAMRAI/SAMRAI_config.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <memory>

#include "SAMRAI/tbox/SAMRAI_MPI.h"
#include "SAMRAI/tbox/PIO.h"
#include "SAMRAI/tbox/SAMRAIManager.h"
#include "SAMRAI/tbox/Utilities.h"
#include "SAMRAI/tbox/MathUtilities.h"

#include "SAMRAI/hier/Box.h"
#include "SAMRAI/hier/BoxContainer.h"
#include "SAMRAI/geom/CartesianGridGeometry.h"
#include "SAMRAI/geom/CartesianPatchGeometry.h"
#include "SAMRAI/pdat/CellIndex.h"
#include "SAMRAI/pdat/CellData.h"
#include "SAMRAI/pdat/CellVariable.h"
#include "SAMRAI/pdat/FaceData.h"
#include "SAMRAI/pdat/FaceIndex.h"
#include "SAMRAI/pdat/FaceVariable.h"
#include "SAMRAI/math/HierarchyCellDataOpsReal.h"
#include "SAMRAI/math/HierarchyDataOpsManager.h"
#include "SAMRAI/math/HierarchyFaceDataOpsReal.h"
#include "SAMRAI/math/HierarchyNodeDataOpsReal.h"
#include "SAMRAI/hier/Box.h"
#include "SAMRAI/hier/Index.h"
#include "SAMRAI/hier/IntVector.h"
#include "SAMRAI/pdat/NodeData.h"
#include "SAMRAI/pdat/NodeIndex.h"
#include "SAMRAI/pdat/NodeVariable.h"
#include "SAMRAI/hier/Patch.h"
#include "SAMRAI/hier/PatchHierarchy.h"
#include "SAMRAI/hier/PatchLevel.h"

#include "SAMRAI/solv/SAMRAIVectorReal.h"
#include "SAMRAI/hier/VariableContext.h"
#include "SAMRAI/hier/VariableDatabase.h"

#ifdef HAVE_SUNDIALS

#include "SAMRAI/solv/Sundials_SAMRAIVector.h"
#include "SAMRAI/solv/SundialsAbstractVector.h"
#include "sundials/sundials_nvector.h"
#include "SAMRAI/solv/solv_NVector.h"

#endif


#define NCELL_VARS 2
#define NFACE_VARS 2
#define NNODE_VARS 4

using namespace SAMRAI;

int main(
   int argc,
   char* argv[]) {

   int fail_count = 0;

   tbox::SAMRAI_MPI::init(&argc, &argv);
   tbox::SAMRAIManager::initialize();
   tbox::SAMRAIManager::startup();

   /*
    * Create block to force pointer deallocation.  If this is not done
    * then there will be memory leaks reported.
    */
   {

      tbox::Dimension dim3d(3);

      tbox::PIO::logOnlyNodeZero("kvtest.log");

#ifdef HAVE_SUNDIALS

      int ln, iv;

      // Make a dummy hierarchy domain
      double lo[3] = { 0.0, 0.0, 0.0 };
      double hi[3] = { 1.0, 0.5, 0.5 };

      const hier::BlockId blk0(0);
      hier::Box coarseSGS(hier::Index(0, 0, 0), hier::Index(4, 2, 2), blk0);

      hier::Box coarse0(hier::Index(0, 0, 0), hier::Index(4, 2, 2), blk0);
      hier::Box coarse1(hier::Index(5, 0, 0), hier::Index(9, 2, 2), blk0);
      hier::Box coarse2(hier::Index(0, 0, 3), hier::Index(4, 2, 4), blk0);
      hier::Box coarse3(hier::Index(5, 0, 3), hier::Index(9, 2, 4), blk0);
      hier::Box coarse4(hier::Index(0, 3, 0), hier::Index(4, 4, 2), blk0);
      hier::Box coarse5(hier::Index(5, 3, 0), hier::Index(9, 4, 2), blk0);
      hier::Box coarse6(hier::Index(0, 3, 3), hier::Index(4, 4, 4), blk0);
      hier::Box coarse7(hier::Index(5, 3, 3), hier::Index(9, 4, 4), blk0);
      hier::Box fine0(hier::Index(4, 4, 4), hier::Index(7, 5, 5), blk0);
      hier::Box fine1(hier::Index(4, 4, 6), hier::Index(7, 5, 7), blk0);
      hier::Box fine2(hier::Index(4, 6, 4), hier::Index(7, 7, 5), blk0);
      hier::Box fine3(hier::Index(4, 6, 6), hier::Index(7, 7, 7), blk0);
      hier::Box fine4(hier::Index(8, 4, 4), hier::Index(13, 5, 5), blk0);
      hier::Box fine5(hier::Index(8, 4, 6), hier::Index(13, 5, 7), blk0);
      hier::Box fine6(hier::Index(8, 6, 4), hier::Index(13, 7, 5), blk0);
      hier::Box fine7(hier::Index(8, 6, 6), hier::Index(13, 7, 7), blk0);
      hier::IntVector ratio(dim3d, 2);

      hier::BoxContainer coarse_domain;
      hier::BoxContainer fine_boxes;
      coarse_domain.pushBack(coarse0);
      coarse_domain.pushBack(coarse1);
      coarse_domain.pushBack(coarse2);
      coarse_domain.pushBack(coarse3);
      coarse_domain.pushBack(coarse4);
      coarse_domain.pushBack(coarse5);
      coarse_domain.pushBack(coarse6);
      coarse_domain.pushBack(coarse7);
      fine_boxes.pushBack(fine0);
      fine_boxes.pushBack(fine1);
      fine_boxes.pushBack(fine2);
      fine_boxes.pushBack(fine3);
      fine_boxes.pushBack(fine4);
      fine_boxes.pushBack(fine5);
      fine_boxes.pushBack(fine6);
      fine_boxes.pushBack(fine7);

      hier::BoxContainer coarse_domain_list(coarse_domain);
      hier::BoxContainer fine_level_list(fine_boxes);
      coarse_domain_list.coalesce();
      fine_level_list.coalesce();

      TBOX_ASSERT(coarse_domain_list.size() == 1);
      TBOX_ASSERT(fine_level_list.size() == 1);

      hier::Box coarse_domain_box(coarse_domain_list.front());
      hier::Box fine_level_box(fine_level_list.front());

      std::shared_ptr<geom::CartesianGridGeometry> geometry(
         new geom::CartesianGridGeometry(
            "CartesianGeometry",
            lo,
            hi,
            coarse_domain));

      std::shared_ptr<hier::PatchHierarchy> hierarchy(
         new hier::PatchHierarchy(
            "PatchHierarchy",
            geometry));

      hierarchy->setMaxNumberOfLevels(2);
      hierarchy->setRatioToCoarserLevel(ratio, 1);

      // Note: For these simple tests we allow at most 2 processors.
      const tbox::SAMRAI_MPI& mpi(tbox::SAMRAI_MPI::getSAMRAIWorld());
      const int nproc = mpi.getSize();
      TBOX_ASSERT(nproc < 3);

      const int n_coarse_boxes = coarse_domain.size();
      const int n_fine_boxes = fine_boxes.size();

      std::shared_ptr<hier::BoxLevel> layer0(
         std::make_shared<hier::BoxLevel>(
            hier::IntVector(dim3d, 1), geometry));
      std::shared_ptr<hier::BoxLevel> layer1(
         std::make_shared<hier::BoxLevel>(ratio, geometry));

      hier::BoxContainer::iterator coarse_domain_itr = coarse_domain.begin();
      for (int ib = 0; ib < n_coarse_boxes; ++ib, ++coarse_domain_itr) {
         if (ib % nproc == layer0->getMPI().getRank()) {
            layer0->addBox(hier::Box(*coarse_domain_itr,
                  hier::LocalId(ib),
                  layer0->getMPI().getRank()));
         }
      }

      hier::BoxContainer::iterator fine_boxes_itr = fine_boxes.begin();
      for (int ib = 0; ib < n_fine_boxes; ++ib, ++fine_boxes_itr) {
         if (ib % nproc == layer1->getMPI().getRank()) {
            layer1->addBox(hier::Box(*fine_boxes_itr,
                  hier::LocalId(ib),
                  layer1->getMPI().getRank()));
         }
      }

      hierarchy->makeNewPatchLevel(0, layer0);
      hierarchy->makeNewPatchLevel(1, layer1);

      tbox::ResourceAllocator allocator =
         tbox::AllocatorDatabase::getDatabase()->getDefaultAllocator();

      // Create instance of hier::Variable database
      hier::VariableDatabase* variable_db = hier::VariableDatabase::getDatabase();
      std::shared_ptr<hier::VariableContext> dummy(
         variable_db->getContext("dummy"));
      const hier::IntVector no_ghosts(dim3d, 0);

      // Make some dummy variables and data on the hierarchy
      std::shared_ptr<pdat::CellVariable<double> > cvar[NCELL_VARS];
      int cvindx[NCELL_VARS];
      cvar[0].reset(new pdat::CellVariable<double>(dim3d, "cvar0",
                                                   allocator,
                                                   2));
      cvindx[0] = variable_db->registerVariableAndContext(
            cvar[0], dummy, no_ghosts);
      cvar[1].reset(new pdat::CellVariable<double>(dim3d, "cvar1",
                                                   allocator,
                                                   2));
      cvindx[1] = variable_db->registerVariableAndContext(
            cvar[1], dummy, no_ghosts);

      std::shared_ptr<pdat::CellVariable<double> > cwgt(
         new pdat::CellVariable<double>(dim3d, "cwgt",
                                        allocator,
                                        1));
      int cwgt_id = variable_db->registerVariableAndContext(
            cwgt, dummy, no_ghosts);

      std::shared_ptr<pdat::FaceVariable<double> > fvar[NFACE_VARS];
      int fvindx[NFACE_VARS];
      fvar[0].reset(new pdat::FaceVariable<double>(dim3d, "fvar0",
                                                   allocator,
                                                   1));
      fvindx[0] = variable_db->registerVariableAndContext(
            fvar[0], dummy, no_ghosts);
      fvar[1].reset(new pdat::FaceVariable<double>(dim3d, "fvar1",
                                                   allocator,
                                                   1));
      fvindx[1] = variable_db->registerVariableAndContext(
            fvar[1], dummy, no_ghosts);

      std::shared_ptr<pdat::FaceVariable<double> > fwgt(
         new pdat::FaceVariable<double>(dim3d, "fwgt",
                                        allocator,
                                        1));
      int fwgt_id = variable_db->registerVariableAndContext(
            fwgt, dummy, no_ghosts);

      std::shared_ptr<pdat::NodeVariable<double> > nvar[NNODE_VARS];
      int nvindx[NNODE_VARS];
      nvar[0].reset(new pdat::NodeVariable<double>(dim3d, "nvar0",
                                                   allocator,
                                                   1));
      nvindx[0] = variable_db->registerVariableAndContext(
            nvar[0], dummy, no_ghosts);
      nvar[1].reset(new pdat::NodeVariable<double>(dim3d, "nvar1",
                                                   allocator,
                                                   1));
      nvindx[1] = variable_db->registerVariableAndContext(
            nvar[1], dummy, no_ghosts);
      nvar[2].reset(new pdat::NodeVariable<double>(dim3d, "nvar2",
                                                   allocator,
                                                   1));
      nvindx[2] = variable_db->registerVariableAndContext(
            nvar[2], dummy, no_ghosts);
      nvar[3].reset(new pdat::NodeVariable<double>(dim3d, "nvar3",
                                                   allocator,
                                                   1));
      nvindx[3] = variable_db->registerVariableAndContext(
            nvar[3], dummy, no_ghosts);

      std::shared_ptr<pdat::NodeVariable<double> > nwgt(
         new pdat::NodeVariable<double>(dim3d, "nwgt",
                                        allocator,
                                        1));
      int nwgt_id = variable_db->registerVariableAndContext(
            nwgt, dummy, no_ghosts);

      for (ln = 0; ln < 2; ++ln) {
         hierarchy->getPatchLevel(ln)->allocatePatchData(cwgt_id);
         hierarchy->getPatchLevel(ln)->allocatePatchData(fwgt_id);
         hierarchy->getPatchLevel(ln)->allocatePatchData(nwgt_id);
      }

      std::shared_ptr<math::HierarchyCellDataOpsReal<double> > cell_ops(
         SAMRAI_SHARED_PTR_CAST<math::HierarchyCellDataOpsReal<double>,
                    math::HierarchyDataOpsReal<double> >(
            math::HierarchyDataOpsManager::getManager()->getOperationsDouble(cwgt, hierarchy)));
      std::shared_ptr<math::HierarchyFaceDataOpsReal<double> > face_ops(
         SAMRAI_SHARED_PTR_CAST<math::HierarchyFaceDataOpsReal<double>,
                    math::HierarchyDataOpsReal<double> >(
            math::HierarchyDataOpsManager::getManager()->getOperationsDouble(fwgt, hierarchy)));
      std::shared_ptr<math::HierarchyNodeDataOpsReal<double> > node_ops(
         SAMRAI_SHARED_PTR_CAST<math::HierarchyNodeDataOpsReal<double>,
                    math::HierarchyDataOpsReal<double> >(
            math::HierarchyDataOpsManager::getManager()->getOperationsDouble(nwgt, hierarchy)));

      TBOX_ASSERT(cell_ops);
      TBOX_ASSERT(face_ops);
      TBOX_ASSERT(node_ops);

      cell_ops->resetLevels(0, 1);
      face_ops->resetLevels(0, 1);
      node_ops->resetLevels(0, 1);

      std::shared_ptr<hier::Patch> patch;
      std::shared_ptr<geom::CartesianPatchGeometry> pgeom;
      std::shared_ptr<pdat::CellData<double> > cdata;
      std::shared_ptr<pdat::FaceData<double> > fdata;
      std::shared_ptr<pdat::NodeData<double> > ndata;

      // Set control volume data for vector components
      hier::Box coarse_fine(fine_level_box);
      coarse_fine.coarsen(ratio);

      // Initialize control volume data for cell-centered components

      for (ln = 0; ln < 2; ++ln) {
         std::shared_ptr<hier::PatchLevel> level(
            hierarchy->getPatchLevel(ln));
         for (hier::PatchLevel::iterator ip(level->begin());
              ip != level->end(); ++ip) {
            patch = *ip;
            pgeom = SAMRAI_SHARED_PTR_CAST<geom::CartesianPatchGeometry,
                               hier::PatchGeometry>(patch->getPatchGeometry());
            TBOX_ASSERT(pgeom);
            const double* dx = pgeom->getDx();
            const double cell_vol = dx[0] * dx[1] * dx[2];
            std::shared_ptr<pdat::CellData<double> > cvdata(
               SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
                  patch->getPatchData(cwgt_id)));
            TBOX_ASSERT(cvdata);
            cvdata->fillAll(cell_vol);
            if (ln == 0) cvdata->fillAll(0.0, (coarse_fine * patch->getBox()));
         }
      }

      // Initialize control volume data for face-centered components
      for (ln = 0; ln < 2; ++ln) {

         std::shared_ptr<hier::PatchLevel> level(
            hierarchy->getPatchLevel(ln));
         for (hier::PatchLevel::iterator ip(level->begin());
              ip != level->end(); ++ip) {
            patch = *ip;
            pgeom = SAMRAI_SHARED_PTR_CAST<geom::CartesianPatchGeometry,
                               hier::PatchGeometry>(patch->getPatchGeometry());
            TBOX_ASSERT(pgeom);
            const double* dx = pgeom->getDx();
            const double face_vol = dx[0] * dx[1] * dx[2];
            std::shared_ptr<pdat::FaceData<double> > data(
               SAMRAI_SHARED_PTR_CAST<pdat::FaceData<double>, hier::PatchData>(
                  patch->getPatchData(fwgt_id)));
            TBOX_ASSERT(data);
            data->fillAll(face_vol);
            pdat::FaceIndex fi(dim3d);
            int plo0 = patch->getBox().lower(0);
            int phi0 = patch->getBox().upper(0);
            int plo1 = patch->getBox().lower(1);
            int phi1 = patch->getBox().upper(1);
            int plo2 = patch->getBox().lower(2);
            int phi2 = patch->getBox().upper(2);
            int ic, jc, kc;
            double bdry_face_factor;
            hier::Box level_box(dim3d);

            if (ln == 0) {
               data->fillAll(0.0, (coarse_fine * patch->getBox()));
               bdry_face_factor = 0.5;
               level_box = coarse_domain_box;
            } else {
               bdry_face_factor = 1.5;
               level_box = fine_level_box;
            }
            //X face boundaries
            if (plo0 == level_box.lower(0)) {
               for (kc = plo2; kc <= phi2; ++kc) {
                  for (jc = plo1; jc <= phi1; ++jc) {
                     fi = pdat::FaceIndex(hier::Index(plo0, jc, kc),
                           pdat::FaceIndex::X,
                           pdat::FaceIndex::Lower);
                     (*data)(fi) *= bdry_face_factor;
                  }
               }
            } else {
               for (kc = plo2; kc <= phi2; ++kc) {
                  for (jc = plo1; jc <= phi1; ++jc) {
                     fi = pdat::FaceIndex(hier::Index(plo0, jc, kc),
                           pdat::FaceIndex::X,
                           pdat::FaceIndex::Lower);
                     (*data)(fi) = 0.0;
                  }
               }
            }
            if (phi0 == level_box.upper(0)) {
               for (kc = plo2; kc <= phi2; ++kc) {
                  for (jc = plo1; jc <= phi1; ++jc) {
                     fi = pdat::FaceIndex(hier::Index(phi0, jc, kc),
                           pdat::FaceIndex::X,
                           pdat::FaceIndex::Upper);
                     (*data)(fi) *= bdry_face_factor;
                  }
               }
            }

            //Y face boundaries
            if (plo1 == level_box.lower(1)) {
               for (kc = plo2; kc <= phi2; ++kc) {
                  for (ic = plo0; ic <= phi0; ++ic) {
                     fi = pdat::FaceIndex(hier::Index(ic, plo1, kc),
                           pdat::FaceIndex::Y,
                           pdat::FaceIndex::Lower);
                     (*data)(fi) *= bdry_face_factor;
                  }
               }
            } else {
               for (kc = plo2; kc <= phi2; ++kc) {
                  for (ic = plo0; ic <= phi0; ++ic) {
                     fi = pdat::FaceIndex(hier::Index(ic, plo1, kc),
                           pdat::FaceIndex::Y,
                           pdat::FaceIndex::Lower);
                     (*data)(fi) = 0.0;
                  }
               }
            }
            if (phi1 == level_box.upper(1)) {
               for (kc = plo2; kc <= phi2; ++kc) {
                  for (ic = plo0; ic <= phi0; ++ic) {
                     fi = pdat::FaceIndex(hier::Index(ic, phi1, kc),
                           pdat::FaceIndex::Y,
                           pdat::FaceIndex::Upper);
                     (*data)(fi) *= bdry_face_factor;
                  }
               }
            }

            //Z face boundaries
            if (plo2 == level_box.lower(2)) {
               for (jc = plo1; jc <= phi1; ++jc) {
                  for (ic = plo0; ic <= phi0; ++ic) {
                     fi = pdat::FaceIndex(hier::Index(ic, jc, plo2),
                           pdat::FaceIndex::Z,
                           pdat::FaceIndex::Lower);
                     (*data)(fi) *= bdry_face_factor;
                  }
               }
            } else {
               for (jc = plo1; jc <= phi1; ++jc) {
                  for (ic = plo0; ic <= phi0; ++ic) {
                     fi = pdat::FaceIndex(hier::Index(ic, jc, plo2),
                           pdat::FaceIndex::Z,
                           pdat::FaceIndex::Lower);
                     (*data)(fi) = 0.0;
                  }
               }
            }
            if (phi2 == level_box.upper(2)) {
               for (jc = plo1; jc <= phi1; ++jc) {
                  for (ic = plo0; ic <= phi0; ++ic) {
                     fi = pdat::FaceIndex(hier::Index(ic, jc, phi2),
                           pdat::FaceIndex::Z,
                           pdat::FaceIndex::Upper);
                     (*data)(fi) *= bdry_face_factor;
                  }
               }
            }
         }
      }

      for (ln = 0; ln < 2; ++ln) {

         std::shared_ptr<hier::PatchLevel> level(
            hierarchy->getPatchLevel(ln));
         for (hier::PatchLevel::iterator ip(level->begin());
              ip != level->end(); ++ip) {
            patch = *ip;
            pgeom = SAMRAI_SHARED_PTR_CAST<geom::CartesianPatchGeometry,
                               hier::PatchGeometry>(patch->getPatchGeometry());
            TBOX_ASSERT(pgeom);
            const double* dx = pgeom->getDx();
            const double node_vol = dx[0] * dx[1] * dx[2];
            std::shared_ptr<pdat::NodeData<double> > data(
               SAMRAI_SHARED_PTR_CAST<pdat::NodeData<double>, hier::PatchData>(
                  patch->getPatchData(nwgt_id)));
            TBOX_ASSERT(data);
            data->fillAll(node_vol);
            pdat::NodeIndex ni(dim3d);
            hier::Index plo = patch->getBox().lower();
            hier::Index phi = patch->getBox().upper();
            int ic, jc, kc;
            double bdry_face_factor;
            double bdry_edge_factor;
            double bdry_node_factor;
            hier::Box level_box(dim3d);

            if (ln == 0) {
               data->fillAll(0.0, (coarse_fine * patch->getBox()));
               bdry_face_factor = 0.5;
               bdry_edge_factor = 0.25;
               bdry_node_factor = 0.125;
               level_box = coarse_domain_box;
            } else {
               bdry_face_factor = 1.5;
               bdry_edge_factor = 2.25;
               bdry_node_factor = 3.375;
               level_box = fine_level_box;
            }

            //X faces
            if (plo(0) == level_box.lower(0)) {
               for (kc = plo(2); kc < phi(2); ++kc) {
                  for (jc = plo(1); jc < phi(1); ++jc) {
                     ni = pdat::NodeIndex(hier::Index(plo(
                                 0), jc, kc), pdat::NodeIndex::LUU);
                     (*data)(ni) *= bdry_face_factor;
                  }
               }
            } else {
               for (kc = plo(2); kc < phi(2); ++kc) {
                  for (jc = plo(1); jc < phi(1); ++jc) {
                     ni = pdat::NodeIndex(hier::Index(plo(
                                 0), jc, kc), pdat::NodeIndex::LUU);
                     (*data)(ni) = 0.0;
                  }
               }
            }
            if (phi(0) == level_box.upper(0)) {
               for (kc = plo(2); kc < phi(2); ++kc) {
                  for (jc = plo(1); jc < phi(1); ++jc) {
                     ni = pdat::NodeIndex(hier::Index(phi(
                                 0), jc, kc), pdat::NodeIndex::UUU);
                     (*data)(ni) *= bdry_face_factor;
                  }
               }
            }

            //Y faces
            if (plo(1) == level_box.lower(1)) {
               for (kc = plo(2); kc < phi(2); ++kc) {
                  for (ic = plo(0); ic < phi(0); ++ic) {
                     ni = pdat::NodeIndex(hier::Index(ic, plo(
                                 1), kc), pdat::NodeIndex::ULU);
                     (*data)(ni) *= bdry_face_factor;
                  }
               }
            } else {
               for (kc = plo(2); kc < phi(2); ++kc) {
                  for (ic = plo(0); ic < phi(0); ++ic) {
                     ni = pdat::NodeIndex(hier::Index(ic, plo(
                                 1), kc), pdat::NodeIndex::ULU);
                     (*data)(ni) = 0.0;
                  }
               }
            }
            if (phi(1) == level_box.upper(1)) {
               for (kc = plo(2); kc < phi(2); ++kc) {
                  for (ic = plo(0); ic < phi(0); ++ic) {
                     ni = pdat::NodeIndex(hier::Index(ic, phi(
                                 1), kc), pdat::NodeIndex::UUU);
                     (*data)(ni) *= bdry_face_factor;
                  }
               }
            }

            //Z faces
            if (plo(2) == level_box.lower(2)) {
               for (jc = plo(1); jc < phi(1); ++jc) {
                  for (ic = plo(0); ic < phi(0); ++ic) {
                     ni = pdat::NodeIndex(hier::Index(ic, jc, plo(
                                 2)), pdat::NodeIndex::UUL);
                     (*data)(ni) *= bdry_face_factor;
                  }
               }
            } else {
               for (jc = plo(1); jc < phi(1); ++jc) {
                  for (ic = plo(0); ic < phi(0); ++ic) {
                     ni = pdat::NodeIndex(hier::Index(ic, jc, plo(
                                 2)), pdat::NodeIndex::UUL);
                     (*data)(ni) = 0.0;
                  }
               }
            }
            if (phi(2) == level_box.upper(2)) {
               for (jc = plo(1); jc < phi(1); ++jc) {
                  for (ic = plo(0); ic < phi(0); ++ic) {
                     ni = pdat::NodeIndex(hier::Index(ic, jc, phi(
                                 2)), pdat::NodeIndex::UUU);
                     (*data)(ni) *= bdry_face_factor;
                  }
               }
            }

            // edge boundaries
            for (ic = plo(0); ic < phi(0); ++ic) {
               ni = pdat::NodeIndex(hier::Index(ic, plo(1), plo(
                           2)), pdat::NodeIndex::ULL);
               if (plo(1) == level_box.lower(1)) {
                  if (plo(2) == level_box.lower(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) = 0.0;
                  }
               } else {
                  (*data)(ni) = 0.0;
               }
               ni = pdat::NodeIndex(hier::Index(ic, phi(1), plo(
                           2)), pdat::NodeIndex::UUL);
               if (phi(1) == level_box.upper(1)) {
                  if (plo(2) == level_box.lower(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) = 0.0;
                  }
               } else {
                  if (plo(2) == level_box.lower(2)) {
                     (*data)(ni) *= bdry_face_factor;
                  } else {
                     (*data)(ni) *= 0.0;
                  }
               }
               ni = pdat::NodeIndex(hier::Index(ic, plo(1), phi(
                           2)), pdat::NodeIndex::ULU);
               if (plo(1) == level_box.lower(1)) {
                  if (phi(2) == level_box.upper(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) *= bdry_face_factor;
                  }
               } else {
                  (*data)(ni) = 0.0;
               }
               ni = pdat::NodeIndex(hier::Index(ic, phi(1), phi(
                           2)), pdat::NodeIndex::UUU);
               if (phi(1) == level_box.upper(1)) {
                  if (phi(2) == level_box.upper(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) *= bdry_face_factor;
                  }
               } else {
                  if (phi(2) == level_box.upper(2)) {
                     (*data)(ni) *= bdry_face_factor;
                  }
               }
            }

            for (jc = plo(1); jc < phi(1); ++jc) {
               ni = pdat::NodeIndex(hier::Index(plo(0), jc, plo(
                           2)), pdat::NodeIndex::LUL);
               if (plo(0) == level_box.lower(0)) {
                  if (plo(2) == level_box.lower(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) = 0.0;
                  }
               } else {
                  (*data)(ni) = 0.0;
               }
               ni = pdat::NodeIndex(hier::Index(phi(0), jc, plo(
                           2)), pdat::NodeIndex::UUL);
               if (phi(0) == level_box.upper(0)) {
                  if (plo(2) == level_box.lower(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) = 0.0;
                  }
               } else {
                  if (plo(2) == level_box.lower(2)) {
                     (*data)(ni) *= bdry_face_factor;
                  } else {
                     (*data)(ni) *= 0.0;
                  }
               }
               ni = pdat::NodeIndex(hier::Index(plo(0), jc, phi(
                           2)), pdat::NodeIndex::LUU);
               if (plo(0) == level_box.lower(0)) {
                  if (phi(2) == level_box.upper(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) *= bdry_face_factor;
                  }
               } else {
                  (*data)(ni) = 0.0;
               }
               ni = pdat::NodeIndex(hier::Index(phi(0), jc, phi(
                           2)), pdat::NodeIndex::UUU);
               if (phi(0) == level_box.upper(0)) {
                  if (phi(2) == level_box.upper(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) *= bdry_face_factor;
                  }
               } else {
                  if (phi(2) == level_box.upper(2)) {
                     (*data)(ni) *= bdry_face_factor;
                  }
               }
            }

            for (kc = plo(2); kc < phi(2); ++kc) {
               ni = pdat::NodeIndex(hier::Index(plo(0), plo(
                           1), kc), pdat::NodeIndex::LLU);
               if (plo(0) == level_box.lower(0)) {
                  if (plo(1) == level_box.lower(1)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) = 0.0;
                  }
               } else {
                  (*data)(ni) = 0.0;
               }
               ni = pdat::NodeIndex(hier::Index(phi(0), plo(
                           1), kc), pdat::NodeIndex::ULU);
               if (phi(0) == level_box.upper(0)) {
                  if (plo(1) == level_box.lower(1)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) = 0.0;
                  }
               } else {
                  if (plo(1) == level_box.lower(1)) {
                     (*data)(ni) *= bdry_face_factor;
                  } else {
                     (*data)(ni) *= 0.0;
                  }
               }
               ni = pdat::NodeIndex(hier::Index(plo(0), phi(
                           1), kc), pdat::NodeIndex::LUU);
               if (plo(0) == level_box.lower(0)) {
                  if (phi(1) == level_box.upper(1)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) *= bdry_face_factor;
                  }
               } else {
                  (*data)(ni) = 0.0;
               }
               ni = pdat::NodeIndex(hier::Index(phi(0), phi(
                           1), kc), pdat::NodeIndex::UUU);
               if (phi(0) == level_box.upper(0)) {
                  if (phi(1) == level_box.upper(1)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) *= bdry_face_factor;
                  }
               } else {
                  if (phi(1) == level_box.upper(1)) {
                     (*data)(ni) *= bdry_face_factor;
                  }
               }
            }

            //corner boundaries
            ni = pdat::NodeIndex(hier::Index(plo(0), plo(1), plo(
                        2)), pdat::NodeIndex::LLL);
            if (plo(0) == level_box.lower(0)) {
               if (plo(1) == level_box.lower(1)) {
                  if (plo(2) == level_box.lower(2)) {
                     (*data)(ni) *= bdry_node_factor;
                  } else {
                     (*data)(ni) = 0.0;
                  }
               } else {
                  (*data)(ni) = 0.0;
               }
            } else {
               (*data)(ni) = 0.0;
            }

            ni = pdat::NodeIndex(hier::Index(plo(0), plo(1), phi(
                        2)), pdat::NodeIndex::LLU);
            if (plo(0) == level_box.lower(0)) {
               if (plo(1) == level_box.lower(1)) {
                  if (phi(2) == level_box.upper(2)) {
                     (*data)(ni) *= bdry_node_factor;
                  } else {
                     (*data)(ni) *= bdry_edge_factor;
                  }
               } else {
                  (*data)(ni) = 0.0;
               }
            } else {
               (*data)(ni) = 0.0;
            }

            ni = pdat::NodeIndex(hier::Index(plo(0), phi(1), plo(
                        2)), pdat::NodeIndex::LUL);
            if (plo(0) == level_box.lower(0)) {
               if (phi(1) == level_box.upper(1)) {
                  if (plo(2) == level_box.lower(2)) {
                     (*data)(ni) *= bdry_node_factor;
                  } else {
                     (*data)(ni) = 0.0;
                  }
               } else {
                  if (plo(2) == level_box.lower(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) = 0.0;
                  }
               }
            } else {
               (*data)(ni) = 0.0;
            }

            ni = pdat::NodeIndex(hier::Index(plo(0), phi(1), phi(
                        2)), pdat::NodeIndex::LUU);
            if (plo(0) == level_box.lower(0)) {
               if (phi(1) == level_box.upper(1)) {
                  if (phi(2) == level_box.upper(2)) {
                     (*data)(ni) *= bdry_node_factor;
                  } else {
                     (*data)(ni) *= bdry_edge_factor;
                  }
               } else {
                  if (phi(2) == level_box.upper(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) *= bdry_face_factor;
                  }
               }
            } else {
               (*data)(ni) = 0.0;
            }

            ni = pdat::NodeIndex(hier::Index(phi(0), plo(1), plo(
                        2)), pdat::NodeIndex::ULL);
            if (phi(0) == level_box.upper(0)) {
               if (plo(1) == level_box.lower(1)) {
                  if (plo(2) == level_box.lower(2)) {
                     (*data)(ni) *= bdry_node_factor;
                  } else {
                     (*data)(ni) *= 0.0;
                  }
               } else {
                  (*data)(ni) = 0.0;
               }
            } else {
               if (plo(1) == level_box.lower(1)) {
                  if (plo(2) == level_box.lower(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) = 0.0;
                  }
               } else {
                  (*data)(ni) = 0.0;
               }
            }

            ni = pdat::NodeIndex(hier::Index(phi(0), plo(1), phi(
                        2)), pdat::NodeIndex::ULU);
            if (phi(0) == level_box.upper(0)) {
               if (plo(1) == level_box.lower(1)) {
                  if (phi(2) == level_box.upper(2)) {
                     (*data)(ni) *= bdry_node_factor;
                  } else {
                     (*data)(ni) *= bdry_edge_factor;
                  }
               } else {
                  (*data)(ni) = 0.0;
               }
            } else {
               if (plo(1) == level_box.lower(1)) {
                  if (phi(2) == level_box.upper(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) *= bdry_face_factor;
                  }
               } else {
                  (*data)(ni) = 0.0;
               }
            }

            ni = pdat::NodeIndex(hier::Index(phi(0), phi(1), plo(
                        2)), pdat::NodeIndex::UUL);
            if (phi(0) == level_box.upper(0)) {
               if (phi(1) == level_box.upper(1)) {
                  if (plo(2) == level_box.lower(2)) {
                     (*data)(ni) *= bdry_node_factor;
                  } else {
                     (*data)(ni) = 0.0;
                  }
               } else {
                  if (plo(2) == level_box.lower(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) = 0.0;
                  }
               }
            } else {
               if (phi(1) == level_box.upper(1)) {
                  if (plo(2) == level_box.lower(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) = 0.0;
                  }
               } else {
                  if (plo(2) == level_box.lower(2)) {
                     (*data)(ni) *= bdry_face_factor;
                  } else {
                     (*data)(ni) = 0.0;
                  }
               }
            }

            ni = pdat::NodeIndex(hier::Index(phi(0), phi(1), phi(
                        2)), pdat::NodeIndex::UUU);
            if (phi(0) == level_box.upper(0)) {
               if (phi(1) == level_box.upper(1)) {
                  if (phi(2) == level_box.upper(2)) {
                     (*data)(ni) *= bdry_node_factor;
                  } else {
                     (*data)(ni) *= bdry_edge_factor;
                  }
               } else {
                  if (phi(2) == level_box.upper(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) *= bdry_face_factor;
                  }
               }
            } else {
               if (phi(1) == level_box.upper(1)) {
                  if (phi(2) == level_box.upper(2)) {
                     (*data)(ni) *= bdry_edge_factor;
                  } else {
                     (*data)(ni) *= bdry_face_factor;
                  }
               } else {
                  if (phi(2) == level_box.upper(2)) {
                     (*data)(ni) *= bdry_face_factor;
                  }
               }
            }
         }
      }

      // Create SAMRAI vectors:
      // Each vector has four components (1 cell component with depth = 2,
      // 1 face component with depth = 1, and 2 node components with depth = 1).
      std::shared_ptr<solv::SAMRAIVectorReal<double> > my_vec0(
         new solv::SAMRAIVectorReal<double>(
            "my_vec0",
            hierarchy,
            0,
            1));
      my_vec0->addComponent(cvar[0], cvindx[0], cwgt_id);
      my_vec0->addComponent(fvar[0], fvindx[0], fwgt_id);
      my_vec0->addComponent(nvar[0], nvindx[0], nwgt_id);
      my_vec0->addComponent(nvar[1], nvindx[1], nwgt_id);

      std::shared_ptr<solv::SAMRAIVectorReal<double> > my_vec1(
         new solv::SAMRAIVectorReal<double>(
            "my_vec1",
            hierarchy,
            0,
            1));
      my_vec1->addComponent(cvar[1], cvindx[1], cwgt_id);
      my_vec1->addComponent(fvar[1], fvindx[1], fwgt_id);
      my_vec1->addComponent(nvar[2], nvindx[2], nwgt_id);
      my_vec1->addComponent(nvar[3], nvindx[3], nwgt_id);

      my_vec0->allocateVectorData();
      my_vec1->allocateVectorData();

      // Print out control volume data and compute integrals...

      tbox::plog << "cell control volume data" << std::endl;
      cell_ops->printData(cwgt_id, tbox::plog);
      tbox::plog << "face control volume data" << std::endl;
      face_ops->printData(fwgt_id, tbox::plog);
      tbox::plog << "node control volume data" << std::endl;
      node_ops->printData(nwgt_id, tbox::plog);

      double norm;
      //pout << "sum of each control volume is " << std::endl;

      norm = cell_ops->sumControlVolumes(cvindx[0], cwgt_id);
      if (!tbox::MathUtilities<double>::equalEps(norm, (double)0.5)) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #1, norm != 0.5\n";
      }
      //pout << "Component 0 : " << norm << " = 0.5?" << std::endl;

      norm = face_ops->sumControlVolumes(fvindx[0], fwgt_id);
      if (!tbox::MathUtilities<double>::equalEps(norm, (double)0.75)) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #2, norm != 0.75\n";
      }
      //pout << "Component 1 : " << norm << " = 0.75?" << std::endl;

      norm = node_ops->sumControlVolumes(nvindx[0], nwgt_id);
      if (!tbox::MathUtilities<double>::equalEps(norm, (double)0.25)) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #3, norm != 0.25\n";
      }
      //pout << "Component 2 : " << norm << " = 0.25?" << std::endl;

      norm = node_ops->sumControlVolumes(nvindx[1], nwgt_id);
      if (!tbox::MathUtilities<double>::equalEps(norm, (double)0.25)) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #4, norm != 0.25\n";
      }
      //pout << "Component 3 : " << norm << " = 0.25?\n" << std::endl;

      // Simple tests of SAMRAI vector operations

      // Construct SAMRAI vector wrappers to test operations via Sundials calls
      N_Vector kvec0 = solv::Sundials_SAMRAIVector::createSundialsVector(
            my_vec0)->getNVector();
      N_Vector kvec1 = solv::Sundials_SAMRAIVector::createSundialsVector(
            my_vec1)->getNVector();

      double zero = 0.0;
      double half = 0.5;
      double one = 1.0;
      double two = 2.0;
      double three = 3.0;
      double four = 4.0;
      double twelve = 12.0;

      // my_vec0 = 2.0
      my_vec0->setToScalar(2.0);
      N_VPrint_SAMRAI(kvec0);

      double my_norm;
      double p_norm;
      my_norm = my_vec0->L1Norm();
      //pout << "L1-norm of my_vec0 is " << norm << " = 6.0?\n" << std::endl;
      p_norm = N_VL1Norm(kvec0);
      //pout << "L1-norm of kvec0 is " << norm << " = 6.0?\n" << std::endl;
      if (!tbox::MathUtilities<double>::equalEps(my_norm, p_norm)) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #5, L1-norm calculation\n";
      }

      // Set fine data in my_vec0 = 3.0
      cell_ops->resetLevels(1, 1);
      face_ops->resetLevels(1, 1);
      node_ops->resetLevels(1, 1);
      cell_ops->setToScalar(cvindx[0], 3.0);
      face_ops->setToScalar(fvindx[0], 3.0);
      node_ops->setToScalar(nvindx[0], 3.0);
      node_ops->setToScalar(nvindx[1], 3.0);

      tbox::plog << "CHECK my_vec0" << std::endl;
      my_vec0->print(tbox::plog);

      double my_min_val = my_vec0->min();

      double p_min_val;
      p_min_val = N_VMin(kvec0);
      tbox::plog << "min of kvec0 is " << p_min_val << " = 2.0?\n" << std::endl;
      if (!tbox::MathUtilities<double>::equalEps(my_min_val, p_min_val)) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #9, min val calculation\n";
      }

      // my_vec1 = 1/my_vec0
      my_vec1->reciprocal(my_vec0);

      double my_max_val = my_vec1->max();

      double p_max_val;
      p_max_val = N_VMaxNorm(kvec1);
      if (!tbox::MathUtilities<double>::equalEps(my_max_val, p_max_val)) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #10, reciprocal max val calculation\n";
      }

      // Manipulate patch data on vector and test norms

      // Set some bogus values on Level in my_vec1 that should be masked out
      // in ensuing vector norm calculations

      std::shared_ptr<hier::PatchLevel> level_zero(
         hierarchy->getPatchLevel(0));
      for (hier::PatchLevel::iterator ip(level_zero->begin());
           ip != level_zero->end(); ++ip) {
         patch = *ip;

         cdata = SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>,
                            hier::PatchData>(patch->getPatchData(cvindx[1]));
         TBOX_ASSERT(cdata);
         hier::Index cindex0(2, 2, 2);
         hier::Index cindex1(5, 3, 2);
         hier::Index cindex2(4, 2, 2);
         hier::Index cindex3(6, 3, 2);
         if (patch->getBox().contains(cindex0)) {
            (*cdata)(pdat::CellIndex(cindex0), 0) = 100.0;
         }
         if (patch->getBox().contains(cindex1)) {
            (*cdata)(pdat::CellIndex(cindex1), 0) = -1000.0;
         }
         if (patch->getBox().contains(cindex2)) {
            (*cdata)(pdat::CellIndex(cindex2), 1) = 1100.0;
         }
         if (patch->getBox().contains(cindex3)) {
            (*cdata)(pdat::CellIndex(cindex3), 1) = -10.0;
         }

         fdata = SAMRAI_SHARED_PTR_CAST<pdat::FaceData<double>,
                            hier::PatchData>(patch->getPatchData(fvindx[1]));
         TBOX_ASSERT(fdata);
         hier::Index findex0(2, 2, 2);
         hier::Index findex1(5, 3, 2);
         if (patch->getBox().contains(findex0)) {
            (*fdata)
            (pdat::FaceIndex(findex0, pdat::FaceIndex::X,
                pdat::FaceIndex::Lower)) = 200.0;
         }
         if (patch->getBox().contains(findex1)) {
            (*fdata)
            (pdat::FaceIndex(findex1, pdat::FaceIndex::Y,
                pdat::FaceIndex::Upper)) = -2000.0;
         }

         hier::Index nindex0(2, 2, 2);
         hier::Index nindex1(5, 3, 2);
         if (patch->getBox().contains(nindex0)) {
            ndata = SAMRAI_SHARED_PTR_CAST<pdat::NodeData<double>,
                               hier::PatchData>(patch->getPatchData(nvindx[2]));
            TBOX_ASSERT(ndata);
            (*ndata)(pdat::NodeIndex(nindex0, pdat::NodeIndex::LLL)) = 300.0;
            ndata = SAMRAI_SHARED_PTR_CAST<pdat::NodeData<double>,
                               hier::PatchData>(patch->getPatchData(nvindx[3]));
            TBOX_ASSERT(ndata);
            (*ndata)(pdat::NodeIndex(nindex0, pdat::NodeIndex::LUL)) = 30.0;
         }
         if (patch->getBox().contains(nindex1)) {
            ndata = SAMRAI_SHARED_PTR_CAST<pdat::NodeData<double>,
                               hier::PatchData>(patch->getPatchData(nvindx[2]));
            TBOX_ASSERT(ndata);
            (*ndata)(pdat::NodeIndex(nindex1, pdat::NodeIndex::UUL)) = -300.0;
            ndata = SAMRAI_SHARED_PTR_CAST<pdat::NodeData<double>,
                               hier::PatchData>(patch->getPatchData(nvindx[3]));
            TBOX_ASSERT(ndata);
            (*ndata)(pdat::NodeIndex(nindex1, pdat::NodeIndex::ULL)) = -3300.0;
         }
      }

      tbox::plog << "my_vec1 = 0.5 (& bogus values) on L0, = 0.3333 on L1?"
                 << std::endl;
      my_vec1->print(tbox::plog);

      double max_val = my_vec1->max();
      if (!tbox::MathUtilities<double>::equalEps(max_val, (double)1100.0)) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #11, max bogus value\n";
      }

      double min_val = my_vec1->min();
      if (!tbox::MathUtilities<double>::equalEps(min_val, (double)-3300.0)) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #12, min bogus value\n";
      }

      double my_dot = my_vec1->dot(my_vec1);

      double p_dot;
      p_dot = N_VDotProd(kvec1, kvec1);
      if (!tbox::MathUtilities<double>::equalEps(my_dot, p_dot)) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #14, dot product calculation\n";
         std::cout << "SGS " << my_dot << "," << p_dot << std::endl;
      }

      my_norm = my_vec1->maxNorm();
      p_norm = N_VMaxNorm(kvec1);
      if (!tbox::MathUtilities<double>::equalEps(my_norm, p_norm)) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #16, max norm calculation\n";
      }

      N_VConst(twelve, kvec0);
      norm = my_vec1->weightedL2Norm(my_vec0);
      if (!tbox::MathUtilities<double>::equalEps(norm, (double)7.6393717)) {
         ++fail_count;
      }
      norm = N_VWL2Norm(kvec1, kvec0);
      if (!tbox::MathUtilities<double>::equalEps(norm, (double)7.6393717)) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #17, weighted L2 norm calculation\n";
      }

      norm = my_vec0->RMSNorm();
      if (!tbox::MathUtilities<double>::equalEps(norm, (double)12.0)) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #18, RMS norm calculation\n";
      }

      norm = my_vec0->weightedRMSNorm(my_vec1);
      if (!tbox::MathUtilities<double>::equalEps(norm,
             (double)5.77482219887084)) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #19, weighted RMS norm calculation\n";
      }
      norm = N_VWrmsNorm(kvec0, kvec1);
      if (!tbox::MathUtilities<double>::equalEps(norm,
             (double)5.77482219887084)) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #19, weighted RMS norm calculation\n";
      }

      // Vector test routines
      int test = my_vec0->testReciprocal(my_vec1);
      if (test != 1) {
         ++fail_count;
         tbox::perr << "FAILED: - Test #20, reciprocal\n";
      }

      // NOTE: -1's on face and node components on level 1 are due to fact
      //       that these entries are redundant and are ignored in the
      //       test calculation.  Thus, they should be ignored in the
      //       operations that follow.

      // Duplicate vectors
      std::shared_ptr<solv::SAMRAIVectorReal<double> > my_vec2(
         my_vec0->cloneVector("my_vec2"));
      my_vec2->allocateVectorData();

      my_vec2->setRandomValues(1.0, 0.0);

      N_Vector kvec2 = solv::Sundials_SAMRAIVector::createSundialsVector(
            my_vec2)->getNVector();

      tbox::plog
      << "\n\nPRINTING VARIABLE DATABASE before adding new vector" << std::endl;
      variable_db->printClassData(tbox::plog);

      N_Vector kvec3 = N_VClone(kvec2);

      std::shared_ptr<solv::SAMRAIVectorReal<double> > sam_vec3(
         solv::Sundials_SAMRAIVector::getSAMRAIVector(kvec3));

      tbox::plog << "\nVariables and data components in new vector...";
      int ncomp = sam_vec3->getNumberOfComponents();
      for (int ic = 0; ic < ncomp; ++ic) {
         tbox::plog << "\n   Comp id, variable, data id = "
                    << ic << ", "
                    << sam_vec3->getComponentVariable(ic)->getName() << ", "
                    << sam_vec3->getComponentDescriptorIndex(ic);
      }

      tbox::plog << "\n\nPRINTING VARIABLE DATABASE after adding new vector"
                 << std::endl;
      variable_db->printClassData(tbox::plog);

      N_VScale(one, kvec2, kvec3);

      tbox::plog << "kvec3 = Random....?" << std::endl;
//      N_VPrint(kvec3);

      N_VScale(one, kvec0, kvec3);
      N_VScale(one, kvec2, kvec0);

      N_VLinearSum(one, kvec0, four, kvec3, kvec0);

      N_VScale(half, kvec3, kvec3);

      N_VConst(one, kvec0);
      N_VConst(two, kvec1);
      N_VConst(three, kvec2);

      N_VLinearSum(three, kvec1, half, kvec0, kvec0);
      tbox::plog << "kvec0 = 3 * 2 + 0.5 * 1 = 6.5?" << std::endl;
//      N_VPrint(kvec0);

      N_VLinearSum(one, kvec0, twelve, kvec1, kvec1);
      tbox::plog << "kvec1 = 6.5 + 12 * 2 = 30.5?" << std::endl;
//      N_VPrint(kvec1);

      N_VLinearSum(zero, kvec0, one, kvec0, kvec0);
      tbox::plog << "kvec0 = 0 * 6.5 + 6.5 = 6.5?" << std::endl;
//      N_VPrint(kvec0);

      N_VProd(kvec2, kvec1, kvec0);
      tbox::plog << "kvec0 = 3.0 * 30.5 = 91.5?" << std::endl;
//      N_VPrint(kvec0);

      N_VDiv(kvec2, kvec1, kvec0);
      tbox::plog << "kvec0 = 3.0 / 30.5 = 0.098360656?" << std::endl;
//      N_VPrint(kvec0);

      my_vec0->setRandomValues(7.0, -5.0);
      tbox::plog << "kvec0 has random values between -5.0 and 2.0" << std::endl;
//      N_VPrint(kvec0);

      N_VAbs(kvec0, kvec0);
      tbox::plog << "all negative values from kvec0 have been made positive"
                 << std::endl;
//      N_VPrint(kvec0);

      N_VInv(kvec1, kvec0);
      tbox::plog << "kvec0 = 1.0 / 30.5 = 0.032786885?" << std::endl;
//      N_VPrint(kvec0);

      N_VAddConst(kvec0, twelve, kvec0);
      tbox::plog << "kvec0 = .032786885 + 12.0 = 12.032786885?" << std::endl;
      N_VPrint_SAMRAI(kvec0);

      my_vec0->setRandomValues(5.0, 0.0);
      N_VCompare(2.5, kvec0, kvec1);
      tbox::plog << "entries of kvec1 are all either 1.0 or 0.0 " << std::endl
                 << "at points with non-zero control volume" << std::endl;
      N_VPrint_SAMRAI(kvec1);

      test = N_VInvTest(kvec2, kvec0);
      tbox::plog << "is kvec0 pointwise inverted kvec2 (1.0/3.0)" << std::endl
                 << "at points with non-zero control volume?" << std::endl;
      if (test == 0) tbox::plog << "kvec1 has at least one zero element"
                                << std::endl;
      N_VPrint_SAMRAI(kvec0);

      my_vec0->setRandomValues(2.0, -1.0);
      N_VConst(two, kvec1);

      // No more tests....Destroy vectors and data...

      N_VDestroy(kvec3);

      tbox::plog
      << "\n\nPRINTING VARIABLE DATABASE after freeing new vector" << std::endl;
      variable_db->printClassData(tbox::plog);

      N_VDestroy(kvec0);
      N_VDestroy(kvec1);
      N_VDestroy(kvec2);

      // Deallocate vector data and control volumes
      my_vec0->freeVectorComponents();
      my_vec1->freeVectorComponents();
      my_vec2->freeVectorComponents();

      for (ln = 0; ln < 2; ++ln) {
         hierarchy->getPatchLevel(ln)->deallocatePatchData(cwgt_id);
         hierarchy->getPatchLevel(ln)->deallocatePatchData(fwgt_id);
         hierarchy->getPatchLevel(ln)->deallocatePatchData(nwgt_id);
      }

      for (iv = 0; iv < NCELL_VARS; ++iv) {
         cvar[iv].reset();
      }
      for (iv = 0; iv < NFACE_VARS; ++iv) {
         fvar[iv].reset();
      }
      for (iv = 0; iv < NNODE_VARS; ++iv) {
         nvar[iv].reset();
      }
      cwgt.reset();
      fwgt.reset();
      nwgt.reset();
      cell_ops.reset();
      face_ops.reset();
      node_ops.reset();

      geometry.reset();
      hierarchy.reset();

#endif

      if (fail_count == 0) {
         tbox::pout << "\nPASSED:  kvtest" << std::endl;
      }
   }

   tbox::SAMRAIManager::shutdown();
   tbox::SAMRAIManager::finalize();
   tbox::SAMRAI_MPI::finalize();

   return fail_count;
}
