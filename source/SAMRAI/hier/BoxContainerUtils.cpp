/*************************************************************************
 *
 * This file is part of the SAMRAI distribution.  For full copyright
 * information, see COPYRIGHT and LICENSE.
 *
 * Copyright:     (c) 1997-2022 Lawrence Livermore National Security, LLC
 * Description:   Common Box operations for Box containers.
 *
 ************************************************************************/
#include "SAMRAI/hier/BoxContainerUtils.h"
#include "SAMRAI/tbox/StartupShutdownManager.h"
#include "SAMRAI/tbox/TimerManager.h"

namespace SAMRAI {
namespace hier {

/*
 * Constructor does nothing because the objects are stateless.
 */

BoxContainerUtils::BoxContainerUtils()
{
}

/*
 ***************************************************************************
 ***************************************************************************
 */

void
BoxContainerUtils::recursivePrintBoxVector(
   const std::vector<Box>& boxes,
   std::ostream& os,
   const std::string& border,
   int detail_depth)
{
   NULL_USE(detail_depth);

   os << border;
   for (std::vector<Box>::const_iterator ni = boxes.begin();
        ni != boxes.end();
        ++ni) {
      os << "  " << *ni;
   }
}

}
}
