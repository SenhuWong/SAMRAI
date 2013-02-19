/*************************************************************************
 *
 * This file is part of the SAMRAI distribution.  For full copyright
 * information, see COPYRIGHT and COPYING.LESSER.
 *
 * Copyright:     (c) 1997-2013 Lawrence Livermore National Security, LLC
 * Description:   Asynchronous Berger-Rigoutsos clustering algorithm.
 *
 ************************************************************************/
#ifndef included_mesh_BergerRigoutsos
#define included_mesh_BergerRigoutsos

#include "SAMRAI/SAMRAI_config.h"

#include "SAMRAI/mesh/BoxGeneratorStrategy.h"
#include "SAMRAI/hier/Connector.h"
#include "SAMRAI/hier/BoxLevel.h"
#include "SAMRAI/hier/PatchLevel.h"
#include "SAMRAI/tbox/AsyncCommStage.h"
#include "SAMRAI/tbox/Database.h"
#include "SAMRAI/tbox/Utilities.h"

#include "boost/shared_ptr.hpp"

namespace SAMRAI {
namespace mesh {

class BergerRigoutsosNode;

/*!
 * @brief Asynchronous Berger-Rigoutsos implementation.
 * This class is derived from the abstract base class
 * mesh::BoxGeneratorStrategy.  Thus, it serves as a concrete
 * implementation of the box generator Strategy pattern interface.
 *
 * This class uses the BergerRigoutsosNode class
 * to carry out the asynchronous Berger-Rigoutsos algorithm.
 * It handles aspects not central to that algorithm.  It:
 * - Implements the box generator Strategy pattern interface.
 * - Provides an interface with the input database for setting
 *   parameters influencing the implementation.
 * - Sorts the output data (if user requests).
 * - Performs some additional error checking.
 * For more details on the parallel implementation,
 * see mesh::BergerRigoutsosNode.
 *
 * <b> Input Parameters </b>
 *
 * <b> Definitions: </b>
 *    - \b max_box_size
 *       The maximum cluster size allowed. This parameter is not critical to
 *       clustering but limiting the cluster size may improve performance of
 *       load balancing algorithms (due to the excessive work required by the
 *       owner of huge clusters).
 *
 *    - \b sort_output_nodes
 *       Whether to sort the output. This makes the normally non-deterministic
 *       ordering deterministic and the results repeatable.
 *
 *    - \b check_min_box_size
 *       A flag to control how to resolve an initial box that violates the
 *       minimum box size. Set to one of these strings: <br>
 *       \b "IGNORE" - violations will be quietly disregarded. <br>
 *       \b "WARN" - violations will cause a warning but the code will
 *       continue anyway. <br>
 *       \b "ERROR" - violations will cause an unrecoverable assertion.
 *
 * <b> Details: </b> <br>
 * <table>
 *   <tr>
 *     <th>parameter</th>
 *     <th>type</th>
 *     <th>default</th>
 *     <th>range</th>
 *     <th>opt/req</th>
 *     <th>behavior on restart</th>
 *   </tr>
 *   <tr>
 *     <td>max_box_size</td>
 *     <td>int[]</td>
 *     <td>all values max int</td>
 *     <td>all values > 0</td>
 *     <td>opt</td>
 *     <td>Not written to restart. Value in input db used.</td>
 *   </tr>
 *   <tr>
 *     <td>sort_output_nodes</td>
 *     <td>bool</td>
 *     <td>FALSE</td>
 *     <td>TRUE, FALSE</td>
 *     <td>opt</td>
 *     <td>Not written to restart. Value in input db used.</td>
 *   </tr>
 *   <tr>
 *     <td>check_min_box_size</td>
 *     <td>string</td>
 *     <td>"WARN"</td>
 *     <td>"WARN", "IGNORE", "ERROR"</td>
 *     <td>opt</td>
 *     <td>Not written to restart. Value in input db used.</td>
 *   </tr>
 * </table>
 *
 *
 * @internal The following are developer inputs useful for experimentation.
 * Defaults are listed in parenthesis:
 *
 * @internal DEV_algo_advance_mode ("ADVANCE_SOME")
 * string
 * Asynchronous algorithm advance mode.  The default has been empirically
 * determined to scale best to higher numbers of processors and work
 * adequately for lower numbers of processors.
 *
 * @internal DEV_owner_mode ("MOST_OVERLAP")
 * string
 * How to chose the owner from a node group.
 * This string is used in BergerRigoutsosNode::setOwnerMode().
 *
 * @internal DEV_min_box_size_from_cutting (all members are 0.0)
 * int[]
 * This is an alternative minimum box size.  It helps reduce excessive box
 * cutting.  If used, a good value is a box with about 3-4 times the volume
 * of the minimum size specified by the findBoxesContainingTags() interface.
 *
 * @internal DEV_max_inflection_cut_from_center (1.0)
 * double
 * Limit the Laplace cut to this fraction of the distance from the center
 * plane to the end.  Zero means cut only at the center plane.  One means
 * unlimited.  Under most situations, one is fine.  A lower setting helps
 * prevent parallel slivers.
 *
 * @internal DEV_inflection_cut_threshold_ar (0.0)
 * double
 * Specifies the mininum box thickness that can be cut, as a ratio to the
 * thinnest box direction.  If the box doesn't have any direction thick
 * enough, then it has a reasonable aspect ratio, so we can cut it in any
 * direction.
 * Degenerate values of DEV_inflection_cut_threshold_ar:
 *    1: cut any direction except the thinnest.
 *    (0,1) and huge values: cut any direction.
 *    0: Not a degenerate case but a special case meaning always cut the
 *       thickest direction.  This leads to more cubic boxes but may
 *       prevent cutting at important feature changes.
 *
 * @internal The following are developer inputs for debugging.  Defaults listed
 * in parenthesis:
 *
 * @internal DEV_log_node_history (false)
 * bool
 * Whether to log what certain actions of nodes in the tree.
 * This degrades the performance but is a very useful debugging tool.
 *
 * @internal DEV_log_cluster_summary (false)
 * bool
 * Whether to briefly log the results of the clustering.
 *
 * @internal DEV_log_cluster (false)
 * bool
 * Whether to log the results of the clustering.
 */
class BergerRigoutsos:public BoxGeneratorStrategy
{

public:
   /*!
    * @brief Constructor.
    */
   explicit BergerRigoutsos(
      const tbox::Dimension& dim,
      const boost::shared_ptr<tbox::Database>& input_db =
         boost::shared_ptr<tbox::Database>());

   /*!
    * @brief Destructor.
    *
    * Deallocate internal data.
    */
   virtual ~BergerRigoutsos();

   /*!
    * @brief Implement the mesh::BoxGeneratorStrategy interface
    * method of the same name.
    *
    * Create a set of boxes that covers all integer tags on
    * the patch level that match the specified tag value.
    * Each box will be at least as large as the given minimum
    * size and the tolerances will be met.
    *
    * The efficiency tolerance is a threshold value for the percentage of
    * tagged cells in each box.  If this percentage is below the tolerance,
    * the box will continue to be split into smaller boxes.
    *
    * The combine tolerance is a threshold value for the sum of the volumes
    * of two boxes into which a box may be potentially split.  If ratio of
    * that sum and the volume of the original box, the box will not be split.
    *
    * @pre !bound_boxes.isEmpty()
    * @pre (tag_level->getDim() == (*(bound_boxes.begin())).getDim()) &&
    *      (tag_level->getDim() == min_box.getDim()) &&
    *      (tag_level->getDim() == max_gcw.getDim())
    */
   void
   findBoxesContainingTags(
      boost::shared_ptr<hier::BoxLevel>& new_box_level,
      boost::shared_ptr<hier::Connector>& tag_to_new,
      const boost::shared_ptr<hier::PatchLevel>& tag_level,
      const int tag_data_index,
      const int tag_val,
      const hier::BoxContainer& bound_boxes,
      const hier::IntVector& min_box,
      const double efficiency_tol,
      const double combine_tol,
      const hier::IntVector& max_gcw);

   /*!
    * @brief Duplicate the MPI communication object for private internal use.
    *
    * A private communicator isolates the complex communications used
    * by the asynchronous algorithm from other communications,
    * protecting this algorithm from un-related communication bugs.
    * Using a duplicated MPI communicator is optional but recommended.
    *
    * Duplicating the communicator is expensive but need only be done
    * once.  All processes in the communicator must participate.  The
    * duplicate communicator is active until this object is destructed
    * or you call this method with MPI_COMM_NULL.
    *
    * When a duplicate MPI communicator is in use, the tag level must
    * be congruent with it.
    */
   void
   useDuplicateMPI(
      const tbox::SAMRAI_MPI& mpi);

   /*!
    * @brief Setup names of timers.
    *
    * By default, timers are named
    * "mesh::BergerRigoutsosNode::*", where the third field is
    * the specific steps performed by the BergerRigoutsosNode.
    * You can override the first two fields with this method.
    * Conforming to the timer naming convention, timer_prefix should
    * have the form "*::*".
    */
   void
   setTimerPrefix(
      const std::string& timer_prefix);

   /*!
    * @brief Get the name of this object.
    */
   const std::string
   getObjectName() const
   {
      return "BergerRigoutsos";
   }


protected:

   /*!
    * @brief Read parameters from input database.
    *
    * @param input_db Input Database.
    */
   void
   getFromInput(
      const boost::shared_ptr<tbox::Database>& input_db);


private:

   /*
    * BergerRigoutsos and BergerRigoutsosNode are tightly coupled.
    * Technically, BergerRigoutsosNode can be made a private subclass
    * of BergerRigoutsos.  BergerRigoutsos has the common parts of the
    * data and algorithm.  BergerRigoutsosNode has the node-specific
    * parts.
    */
   friend class BergerRigoutsosNode;

   /*!
    * @brief How to choose owner for a new box.
    */
   enum OwnerMode { SINGLE_OWNER = 0,
                    MOST_OVERLAP = 1,
                    FEWEST_OWNED = 2,
                    LEAST_ACTIVE = 3 };

   /*!
    * @brief Method for advancing the algorithm.
    *
    * Each corresponds to a choice permitted by setAlgorithmAdvanceMode().
    */
   enum AlgoAdvanceMode { ADVANCE_ANY,
                          ADVANCE_SOME,
                          SYNCHRONOUS };

   typedef std::set<int> IntSet;

   typedef std::vector<int> VectorOfInts;

   //@{
   //! @name Algorithm mode settings

   /*!
    * @brief Set the mode for advancing the asynchronous implementation.
    *
    * Choices are:
    * - @b "SYNCHRONOUS" --> wait for each communication stage to complete
    *   before moving on, thus resulting in synchronous execution.
    * - @b "ADVANCE_ANY" --> advance a node through its
    *   communication stage by using tbox::AsyncCommStage::advanceAny().
    * - @b "ADVANCE_SOME" --> advance a node through its
    *   communication stage by using tbox::AsyncCommStage::advanceSome().
    *
    * The default is "ADVANCE_SOME".
    *
    * Asynchronous modes are NOT guaranteed to compute the output
    * nodes in any particular order.  The order depends on
    * the ordering of message completion, which is not deterministic.
    * If you require consistent outputs, we suggest you have a scheme
    * for reordering the output boxes.
    *
    * @pre (algo_advance_mode == "ADVANCE_ANY") ||
    *      (algo_advance_mode == "ADVANCE_SOME") ||
    *      (algo_advance_mode == "SYNCHRONOUS")
    */
   void
   setAlgorithmAdvanceMode(
      const std::string& algo_advance_mode);

   /*!
    * @brief Set the method for choosing the owner.
    * Choices:
    * - "MOST_OVERLAP"
    *   Ownership is given to the processor with the most
    *   overlap on the candidate box.  Default.
    * - "SINGLE_OWNER"
    *   In single-owner mode, the initial owner (process 0)
    *   always participates and owns all nodes.
    * - "FEWEST_OWNED"
    *   Choose the processor that owns the fewest
    *   nodes when the choice is made.  This is meant to
    *   relieve bottle-necks caused by excessive ownership.
    *   This option may lead to non-deterministic ownerships.
    * - "LEAST_ACTIVE"
    *   Choose the processor that participates in the fewest
    *   number of nodes when the choice is made.
    *   This is meant to relieve bottle-necks caused by
    *   excessive participation. This option may lead to
    *   non-deterministic ownerships.
    *
    * Experiments show that "MOST_OVERLAP" gives the best
    * clustering speed, while "SINGLE_OWNER" may give a faster
    * output globalization (since you don't need an all-gather).
    *
    * @pre (mode == "SINGLE_OWNER") ||(mode == "MOST_OVERLAP") ||
    *      (mode == "FEWEST_OWNED") ||(mode == "LEAST_ACTIVE")
    */
   void
   setOwnerMode(
      const std::string& mode);

   //@}

   /*!
    * @brief Check the congruency between d_mpi and d_tag_level's MPI.
    */
   bool checkMPICongruency() const;

   /*!
    * @brief Set up data that depend on the MPI communicator being
    * used.
    */
   void setupMPIDependentData();

   /*!
    * @brief Run the clustering algorithm to generate the new BoxLevel
    * and compute relationships (if specified by setComputeRelationships()).
    *
    * Sets d_new_box_level and d_tag_to_new.
    */
   void
   clusterAndComputeRelationships();

   //! @brief Participants send new relationship data to node owners.
   void shareNewNeighborhoodSetsWithOwners();

   const tbox::Dimension &getDim() const {
      return d_tag_level->getDim();
   }


   /*!
    * @brief Relationship computation flag.
    *
    * Valid mode values to set are:
    *
    * - "NONE" = No relationship computation.
    *
    * - "TAG_TO_NEW": Compute tag--->new.
    *
    * - "BIDIRECTIONAL": Compute both tag<==>new.
    *
    * The ghost_cell_width specifies the overlap Connector width.
    *
    * By default, compute bidirectional relationships with a ghost cell width
    * of 1.
    *
    * @pre (mode == "NONE") || (mode == "TAG_TO_NEW") ||
    *      (mode == "BIDIRECTIONAL")
    * @pre ghost_cell_width >= hier::IntVector::getZero(d_common->getDim())
    */
   void
   setComputeRelationships(
      const std::string mode,
      const hier::IntVector& ghost_cell_width);

   /*!
    * @brief Sort boxes in d_new_box_level and update d_tag_to_new.
    */
   void
   sortOutputBoxes();

   /*!
    * @brief Sanity check on the private communicator, if it is used.
    *
    * @see useDuplicateMPI().
    */
   void
   assertNoMessageForPrivateCommunicator() const;


   //@{
   //! @name Counter methods.

   void resetCounters();
   void writeCounters();

   void incNumNodesConstructed() { ++d_num_nodes_constructed; }

   void incNumNodesExisting() {
      ++d_num_nodes_existing;
      d_max_nodes_existing =
         tbox::MathUtilities<int>::Max(d_num_nodes_existing, d_max_nodes_existing);
   }
   void incNumNodesActive() {
      ++d_num_nodes_active;
      d_max_nodes_active =
         tbox::MathUtilities<int>::Max(d_num_nodes_active, d_max_nodes_active);
   }
   void incNumNodesOwned() {
      ++d_num_nodes_owned;
      d_max_nodes_owned =
         tbox::MathUtilities<int>::Max(d_num_nodes_owned, d_max_nodes_owned);
   }
   void incNumNodesCommWait() {
      ++d_num_nodes_commwait;
      d_max_nodes_commwait =
         tbox::MathUtilities<int>::Max(d_num_nodes_commwait, d_max_nodes_commwait);
   }
   void incNumNodesCompleted() {
      ++d_num_nodes_completed;
   }
   void incNumContinues(int num_continues) {
      d_num_conts_to_complete += num_continues;
      d_max_conts_to_complete =
         tbox::MathUtilities<int>::Max(d_max_conts_to_complete, num_continues);
   }

   void decNumNodesConstructed() { --d_num_nodes_constructed; }
   void decNumNodesExisting() { --d_num_nodes_existing; }
   void decNumNodesActive() { --d_num_nodes_active; }
   void decNumNodesOwned() { --d_num_nodes_owned; }
   void decNumNodesCommWait() { --d_num_nodes_commwait; }

   //@}


   const tbox::Dimension d_dim;

   //@{
   //@name Parameters from clustering algorithm virtual interface
   int d_tag_data_index;
   int d_tag_val;
   hier::IntVector d_min_box;
   double d_efficiency_tol;
   double d_combine_tol;
   hier::IntVector d_max_gcw;
   //@}

   /*!
    * @brief Level where tags live.
    */
   boost::shared_ptr<hier::PatchLevel> d_tag_level;

   /*!
    * @brief New BoxLevel generated by BR.
    *
    * This is where we store the boxes as we progress in the BR algorithm.
    *
    * This is set in the public clusterAndComputeRelationships() method.
    */
   boost::shared_ptr<hier::BoxLevel> d_new_box_level;

   /*!
    * @brief Connector from tag_box_level to new_box_level.
    *
    * This is where we store the relationships resulting from the BR
    * algorithm.  The relationships are created locally for local nodes in
    * tag_box_level.
    *
    * This is set in the public clusterAndComputeRelationships method.
    */
   boost::shared_ptr<hier::Connector> d_tag_to_new;

   //! @brief Initial boxes for top-down clustering.
   hier::BoxContainer d_root_boxes;

   //! @brief Max box size constraint.
   hier::IntVector d_max_box_size;

   //! @brief Max distance from center for inflection cut.
   double d_max_inflection_cut_from_center;

   //! @brief Threshold for avoiding thinner directions for Laplace cut.
   double d_inflection_cut_threshold_ar;

   //! @brief How to advance asynchronously.
   AlgoAdvanceMode d_algo_advance_mode;

   //! @brief How to chose the group's owner.
   OwnerMode d_owner_mode;

   /*!
    * @brief Relationship computation flag.
    *
    * See setComputeRelationships().
    * - 0 = NONE
    * - 1 = TAG_TO_NEW
    * - 2 = BIDIRECTIONAL
    */
   int d_compute_relationships;

   //! @brief Whether to sort results to make them deterministic.
   bool d_sort_output_nodes;

   /*!
    * @brief Queue on which to append jobs to be
    * launched or relaunched.
    */
   std::list<BergerRigoutsosNode *> d_relaunch_queue;

   /*!
    * @brief Stage handling multiple asynchronous communication groups.
    */
   tbox::AsyncCommStage d_comm_stage;

   /*!
    * @brief Alternate minimum box size applying to inflection
    * point cuts.
    *
    * This size can be greater than the absolute min_size
    * specified by the
    * BoxGeneratorStrategy::findBoxesContainingTags() abstract
    * interface.
    */
   hier::IntVector d_min_box_size_from_cutting;

   /*!
    * @brief List of processes that will send neighbor data
    * for locally owned boxes after the BR algorithm completes.
    */
   IntSet d_relationship_senders;

   /*!
    * @brief Outgoing messages to be sent to node owners
    * describing new relationships found by local process.
    */
   std::map<int, VectorOfInts> d_relationship_messages;

   //@{
   //! @name Communication parameters
   /*!
    * @brief MPI communicator used in communications by the algorithm.
    *
    * @see useDuplicateMPI().
    */
   tbox::SAMRAI_MPI d_mpi_object;
   //! @brief Upperbound of valid tags.
   int d_tag_upper_bound;
   //! @brief Smallest unclaimed MPI tag in pool given to local process.
   int d_available_mpi_tag;
   //@}


   //@{
   //! @name Auxiliary data for analysis and debugging.

   //! @brief Whether to log major actions of primary do loop.
   bool d_log_do_loop;

   //! @brief Whether to log major actions of nodes.
   bool d_log_node_history;

   //! @brief Whether to briefly log cluster summary.
   bool d_log_cluster_summary;

   //! @brief Whether to log cluster summary.
   bool d_log_cluster;

   //! @brief How to resolve initial boxes smaller than min box size.
   char d_check_min_box_size;

   //! @brief Number of tags.
   int d_num_tags_in_all_nodes;

   //! @brief Max number of tags owned.
   int d_max_tags_owned;

   //! @brief Number of nodes constructed.
   int d_num_nodes_constructed;

   //! @brief Current number of nodes existing.
   int d_num_nodes_existing;
   //! @brief Highest number of nodes existing.
   int d_max_nodes_existing;

   //! @brief Current number of nodes active.
   int d_num_nodes_active;
   //! @brief Highest number of nodes active.
   int d_max_nodes_active;

   //! @brief Current number of nodes owned.
   int d_num_nodes_owned;
   //! @brief Highest number of nodes owned.
   int d_max_nodes_owned;

   //! @brief Current number of nodes in communication wait.
   int d_num_nodes_commwait;
   //! @brief Highest number of nodes in communication wait.
   int d_max_nodes_commwait;

   //! @brief Current number of completed.
   int d_num_nodes_completed;

   //! @brief Highest number of generation.
   int d_max_generation;

   //! @brief Current number of boxes generated.
   int d_num_boxes_generated;

   //! @brief Number of continueAlgorithm calls for to complete nodes.
   int d_num_conts_to_complete;

   //! @brief Highest number of continueAlgorithm calls to complete nodes.
   int d_max_conts_to_complete;

   //@}


   //@{
   //! @name Used for evaluating performance;
   bool d_barrier_before;
   bool d_barrier_after;
   //@}


   //@{
   //! @name Performance timer data for this class.

   /*
    * @brief Structure of timers used by this class.
    *
    * Each object can set its own timer names through
    * setTimerPrefix().  This leads to many timer look-ups.  Because
    * it is expensive to look up timers, this class caches the timers
    * that has been looked up.  Each TimerStruct stores the timers
    * corresponding to a prefix.
    */
   struct TimerStruct {
      boost::shared_ptr<tbox::Timer> t_barrier_before;
      boost::shared_ptr<tbox::Timer> t_barrier_after;

      boost::shared_ptr<tbox::Timer> t_find_boxes_containing_tags;
      boost::shared_ptr<tbox::Timer> t_cluster;
      boost::shared_ptr<tbox::Timer> t_cluster_and_compute_relationships;
      boost::shared_ptr<tbox::Timer> t_continue_algorithm;
      boost::shared_ptr<tbox::Timer> t_compute;
      boost::shared_ptr<tbox::Timer> t_comm_wait;
      boost::shared_ptr<tbox::Timer> t_MPI_wait;
      boost::shared_ptr<tbox::Timer> t_compute_new_neighborhood_sets;
      boost::shared_ptr<tbox::Timer> t_share_new_relationships;
      boost::shared_ptr<tbox::Timer> t_share_new_relationships_send;
      boost::shared_ptr<tbox::Timer> t_share_new_relationships_recv;
      boost::shared_ptr<tbox::Timer> t_share_new_relationships_unpack;
      boost::shared_ptr<tbox::Timer> t_local_tasks;
      boost::shared_ptr<tbox::Timer> t_local_histogram;
      /*
       * Multi-stage timers.  These are used in continueAlgorithm()
       * instead of the methods they time, because what they time may
       * include waiting for messages.  They are included in the
       * timer t_continue_algorithm.  They provide timing breakdown
       * for the different stages.
       */
      boost::shared_ptr<tbox::Timer> t_reduce_histogram;
      boost::shared_ptr<tbox::Timer> t_bcast_acceptability;
      boost::shared_ptr<tbox::Timer> t_gather_grouping_criteria;
      boost::shared_ptr<tbox::Timer> t_bcast_child_groups;
      boost::shared_ptr<tbox::Timer> t_bcast_to_dropouts;

      boost::shared_ptr<tbox::Timer> t_global_reductions;
      boost::shared_ptr<tbox::Timer> t_logging;
      boost::shared_ptr<tbox::Timer> t_sort_output_nodes;
   };

   //! @brief Default prefix for Timers.
   static const std::string s_default_timer_prefix;

   /*!
    * @brief Static container of timers that have been looked up.
    */
   static std::map<std::string, TimerStruct> s_static_timers;

   /*!
    * @brief Structure of timers in s_static_timers, matching this
    * object's timer prefix.
    */
   TimerStruct* d_object_timers;

   //@}


};

}
}

#endif  // included_mesh_BergerRigoutsos
