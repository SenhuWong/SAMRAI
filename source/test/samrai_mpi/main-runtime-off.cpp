/*************************************************************************
 *
 * This file is part of the SAMRAI distribution.  For full copyright
 * information, see COPYRIGHT and LICENSE.
 *
 * Copyright:     (c) 1997-2022 Lawrence Livermore National Security, LLC
 * Description:   Test program for SAMRAI_MPI with run-time MPI off.
 *
 ************************************************************************/
#include "SAMRAI/SAMRAI_config.h"

#include "SAMRAI/tbox/PIO.h"
#include "SAMRAI/tbox/SAMRAI_MPI.h"
#include "mpi-interface-tests.h"

using namespace SAMRAI;
using namespace tbox;

/*
 ************************************************************************
 * Test run-time MPI mode off.
 ************************************************************************
 */

int main(
   int argc,
   char* argv[])
{
   int fail_count = 0;

   bool runtime_mpi = false;
   bool disable_mpi = false;
   mpiInterfaceTests(argc, argv, fail_count, runtime_mpi, disable_mpi);

   if (fail_count == 0) {
      tbox::pout << "\nPASSED:  " << argv[0] << std::endl;
   }

   return fail_count;
}
