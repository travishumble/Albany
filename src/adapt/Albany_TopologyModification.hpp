//*****************************************************************//
//    Albany 2.0:  Copyright 2012 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//


#if !defined(Albany_TopologyMOdification_hpp)
#define Albany_TopologyMOdification_hpp

#include <Teuchos_RCP.hpp>
#include <Teuchos_ParameterList.hpp>

#include <Phalanx.hpp>
#include <PHAL_Workset.hpp>
#include <PHAL_Dimension.hpp>

#include "Albany_AbstractAdapter.hpp"
// Uses LCM Topology util class
// Note that all topology functions are in Albany::LCM namespace
#include "Topology.h"
#include "Fracture.h"
#include "Albany_STKDiscretization.hpp"

namespace Albany {

  ///
  /// \brief Topology modification based adapter
  ///
  class TopologyMod : public AbstractAdapter {
  public:

    ///
    /// Constructor
    ///
    TopologyMod(const Teuchos::RCP<Teuchos::ParameterList>& params_,
                const Teuchos::RCP<ParamLib>& paramLib_,
                Albany::StateManager& StateMgr_,
                const Teuchos::RCP<const Epetra_Comm>& comm_);

    ///
    /// Destructor
    ///
    ~TopologyMod();

    ///
    /// Check adaptation criteria to determine if the mesh needs
    /// adapting
    ///
    virtual
    bool
    queryAdaptationCriteria();

    ///
    /// Apply adaptation method to mesh and problem. Returns true if adaptation is performed successfully.
    ///
    virtual
    bool
    adaptMesh(const Epetra_Vector& solution, const Epetra_Vector& ovlp_solution);

    ///
    /// Transfer solution between meshes.
    ///
    virtual
    void
    solutionTransfer(const Epetra_Vector& old_solution,
                     Epetra_Vector& new_solution);

    ///
    /// Each adapter must generate it's list of valid parameters
    ///
    Teuchos::RCP<const Teuchos::ParameterList>
    getValidAdapterParameters() const;

  private:

    ///
    /// Disallow copy and assignment and default
    ///
    TopologyMod();
    TopologyMod(const TopologyMod &);
    TopologyMod &operator=(const TopologyMod &);

    ///
    /// Connectivity display method
    ///
    void showElemToNodes();

    ///
    /// Relation display method
    ///
    void showRelations();

    ///
    /// Parallel all-reduce function. Returns the argument in serial,
    /// returns the sum of the argument in parallel
    int  accumulateFractured(int num_fractured);

    ///
    /// Average stress magnitude in the mesh elements, used for
    /// separation metric
    ///
    std::vector<std::vector<double> > avg_stresses_;

    /// Parallel all-gatherv function. Communicates local open list to
    /// all processors to form global open list.
    void getGlobalOpenList( std::map<stk::mesh::EntityKey, bool>& local_entity_open,
                            std::map<stk::mesh::EntityKey, bool>& global_entity_open);

    ///
    /// stk_mesh Bulk Data
    ///
    stk::mesh::BulkData* bulk_data_;

    Teuchos::RCP<Albany::AbstractSTKMeshStruct> stk_mesh_struct_;

    Teuchos::RCP<Albany::AbstractDiscretization> discretization_;

    Albany::STKDiscretization * stk_discretization_;

    stk::mesh::fem::FEMMetaData * meta_data_;

    stk::mesh::EntityRank node_rank_;
    stk::mesh::EntityRank edge_rank_;
    stk::mesh::EntityRank face_rank_;
    stk::mesh::EntityRank element_rank_;

    Teuchos::RCP<LCM::AbstractFractureCriterion> fracture_criterion_;
    Teuchos::RCP<LCM::Topology> topology_;

    //! Edges to fracture the mesh on
    std::vector<stk::mesh::Entity*> fractured_faces_;

    //! Data structures used to transfer solution between meshes
    //! Element to node connectivity for old mesh

    std::vector<std::vector<stk::mesh::Entity*> > old_elem_to_node_;

    //! Element to node connectivity for new mesh
    std::vector<std::vector<stk::mesh::Entity*> > new_elem_to_node_;

    int num_dim_;
    int remesh_file_index_;
    std::string base_exo_filename_;

  };

}

#endif //ALBANY_TOPOLOGYMOD_HPP
