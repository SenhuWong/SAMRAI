/*************************************************************************
 *
 * This file is part of the SAMRAI distribution.  For full copyright
 * information, see COPYRIGHT and COPYING.LESSER.
 *
 * Copyright:     (c) 1997-2011 Lawrence Livermore National Security, LLC
 * Description:   Collects and writes out data on communication graphs.
 *
 ************************************************************************/
#ifndef included_tbox_CommGraphWriter
#define included_tbox_CommGraphWriter

#include "SAMRAI/SAMRAI_config.h"

#include "SAMRAI/tbox/SAMRAI_MPI.h"

#include <string>
#include <vector>

namespace SAMRAI {
namespace tbox {

/*!
 * @brief Collects data on distributed communication graphs and writes
 * out for analysis.
 *
 * A node can have multiple values, each with a label.  An node can
 * have multiple edges, each with a label.
 */
class CommGraphWriter
{

public:
   /*!
    * @brief Default constructor.
    */
   CommGraphWriter();

   /*!
    * @brief Destructor.
    */
   virtual ~CommGraphWriter();

   /*!
    * @brief Add a graph record.
    *
    * @param[i] mpi Where the graph data is distributed.
    *
    * @param[i] root_rank Process responsible for writing the graph
    *
    * @return Index of the record.
    */
   size_t addRecord(
      const SAMRAI_MPI &mpi,
      int root_rank,
      size_t number_of_edges,
      size_t number_of_node_values );

   /*!
    * @brief Get the current number of records.
    *
    * @return Current number of records.
    */
   size_t getNumberOfRecords() const {
      return d_records.size();
   }

   enum EdgeDirection { FROM=0, TO=1};


   /*!
    * @brief Set an edge in the current record.
    *
    * The label only matters on the root process.  Other processes do
    * nothing in this method.
    */
   void setEdgeInCurrentRecord(
      size_t edge_index,
      const std::string &edge_label,
      double edge_value,
      EdgeDirection edge_direction,
      int other_node );


   /*!
    * @brief Set a node value in the current record.
    *
    * The label only matters on the root process.  Other processes do
    * nothing in this method.
    */
   void setNodeValueInCurrentRecord(
      size_t nodevalue_index,
      const std::string &nodevalue_label,
      double node_value );


   /*!
    * @brief Gather data onto the root process and write out text file.
    */
   void writeGraphToTextStream(
      size_t record_number,
      std::ostream &os ) const;

private:

   struct Edge {
      Edge() : d_value(-1.0), d_dir(TO), d_other_node(-1) {}
      double d_value;
      EdgeDirection d_dir;
      int d_other_node;
      std::string d_label;
   };

   struct NodeValue {
      NodeValue() : d_value(0.0) {}
      double d_value;
      std::string d_label;
   };

   struct Record {
      Record() : d_mpi(MPI_COMM_NULL) {}
      SAMRAI_MPI d_mpi;
      int d_root_rank;
      std::vector<Edge> d_edges;
      std::vector<NodeValue> d_node_values;
   };

   std::vector<Record> d_records;

};

}
}

#endif  // included_tbox_CommGraphWriter
