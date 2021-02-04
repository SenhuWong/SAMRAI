/*************************************************************************
 *
 * This file is part of the SAMRAI distribution.  For full copyright
 * information, see COPYRIGHT and LICENSE.
 *
 * Copyright:     (c) 1997-2021 Lawrence Livermore National Security, LLC
 * Description:   Simple integer id and namestring variable context
 *
 ************************************************************************/

#ifndef included_hier_VariableContext
#define included_hier_VariableContext

#include "SAMRAI/SAMRAI_config.h"

#include <string>

namespace SAMRAI {
namespace hier {

/**
 * Class VariableContext is a simple class that is used to manage
 * variable storage in a meaningful, customizable manner.  A variable
 * context has a name std::string and a unique integer instance identifier.
 * Context objects are typically generated and used by the variable database
 * for mapping names to storage locations, but they may also be created
 * independently of the variable database.
 *
 * SAMRAI applications use contexts and the variable database to manage
 * patch storage locations for variables.  For example, an integration
 * algorithm may require multiple unique contexts to manage storage
 * for a problem variable, such as "OLD" and "NEW".  The algorithm
 * interacts with the database to obtain the contexts and the mapping
 * between the context and the patch descriptor index to access data on the
 * patch hierarchy.
 *
 * \verbatim
 * Important note:
 *
 *    It is strongly recommended that context objects be generated by
 *    the variable database and obtained via the getContext() function.
 *    While contexts can be created by using the variable context
 *    constructor directly and used with the database, this may produce
 *    unexpected results due to potentially improperly generated and name
 *    std::string context identifiers.
 *
 * \endverbatim
 *
 * @see VariableDatabase
 */

class VariableContext
{
public:
   /**
    * Return the current maximum instance number over all variable
    * context objects.  The instance identifiers returned from variable
    * context objects are guaranteed to be between 0 and this number minus
    * one.  Note that this number changes as new variable contexts are created.
    */
   static int
   getCurrentMaximumInstanceNumber()
   {
      return s_instance_counter;
   }

   /**
    * The variable context constructor creates a context with the given
    * name and increments the context index counter.
    *
    * @pre !name.empty()
    */
   explicit VariableContext(
      const std::string& name);

   /**
    * The destructor does nothing interesting.
    */
   ~VariableContext();

   /**
    * Return integer index for VariableContext object.
    */
   int
   getIndex() const
   {
      return d_index;
   }

   /**
    * Return name std::string identifier for VariableContext object.
    */
   const std::string&
   getName() const
   {
      return d_name;
   }

   /**
    * Check whether two contexts are the same.  Return true if the
    * index of the argument context matches the index of this context object.
    * Otherwise, return false.
    */
   bool
   operator == (
      const VariableContext& other) const
   {
      return d_index == other.d_index;
   }

private:
   VariableContext(
      const VariableContext&);                // not implemented
   VariableContext&
   operator = (
      const VariableContext&);                     // not implemented

   static int s_instance_counter;

   std::string d_name;
   int d_index;

};

}
}

#endif
