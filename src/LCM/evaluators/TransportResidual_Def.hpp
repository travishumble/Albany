//*****************************************************************//
//    Albany 2.0:  Copyright 2012 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//

#include <Teuchos_TestForException.hpp>
#include <Phalanx_DataLayout.hpp>
#include <typeinfo>

namespace LCM {

  //**********************************************************************
  template<typename EvalT, typename Traits>
  HDiffusionDeformationMatterResidual<EvalT, Traits>::
  HDiffusionDeformationMatterResidual(const Teuchos::ParameterList& p,
                                      const Teuchos::RCP<Albany::Layouts>& dl) :
    w_bf_(p.get<std::string>("Weighted BF Name"), dl->node_qp_scalar),
    w_grad_bf_(p.get<std::string>("Weighted Gradient BF Name"), dl->node_qp_vector),
    d_star_(p.get<std::string>("Effective Diffusivity Name"), dl->qp_scalar),
    dl_(p.get<std::string>("Diffusion Coefficient Name"), dl->qp_scalar),
    c_lattice_(p.get<std::string>("QP Variable Name"), dl->qp_scalar),
    eqps_(p.get<std::string>("eqps Name"), dl->qp_scalar),
    eqps_factor_(p.get<std::string>("Strain Rate Factor Name"), dl->qp_scalar),
    c_trapped_(p.get<std::string>("Trapped Concentration Name"), dl->qp_scalar),
    n_trapped_(p.get<std::string>("Trapped Solvent Name"), dl->qp_scalar),
    cl_grad_(p.get<std::string>("Gradient QP Variable Name"), dl->qp_vector),
    stress_grad_(p.get<std::string>("Gradient Hydrostatic Stress Name"), dl->qp_vector),
    stab_parameter_(p.get<std::string>("Material Property Name"), dl->qp_scalar),
    def_grad_(p.get<std::string>("Deformation Gradient Name"), dl->qp_tensor),
    p_stress_(p.get<std::string>("Stress Name"), dl->qp_tensor),
    weights_(p.get<std::string>("Weights Name"), dl->qp_scalar),
    tau_factor_(p.get<std::string>("Tau Contribution Name"), dl->qp_scalar),
    element_length_(p.get<std::string>("Element Length Name"), dl->qp_scalar),
    delta_time_(p.get<std::string>("Delta Time Name"), dl->workset_scalar),
    residual_(p.get<std::string>("Residual Name"), dl->node_scalar)
  {
    this->addDependentField(stabParameter);
    this->addDependentField(elementLength);
    this->addDependentField(w_bf_);
    this->addDependentField(w_grad_bf_);
    this->addDependentField(Dstar);
    this->addDependentField(DL);
    this->addDependentField(Clattice);
    this->addDependentField(eqps);
    this->addDependentField(eqpsFactor);
    this->addDependentField(Ctrapped);
    this->addDependentField(Ntrapped);
    this->addDependentField(CLGrad);
    this->addDependentField(stressGrad);
    this->addDependentField(DefGrad);
    this->addDependentField(Pstress);
    this->addDependentField(weights);
    this->addDependentField(tauFactor);
    this->addDependentField(deltaTime);

    //   if (haveSource) this->addDependentField(Source);
    //   if (haveMechSource) this->addDependentField(MechSource);



    this->addEvaluatedField(residual_);


    Teuchos::RCP<PHX::DataLayout> vector_dl =
      p.get< Teuchos::RCP<PHX::DataLayout> >("QP Vector Data Layout");
    std::vector<PHX::DataLayout::size_type> dims;
    vector_dl->dimensions(dims);
    numQPs  = dims[1];
    numDims = dims[2];

    Teuchos::RCP<PHX::DataLayout> node_dl =
      p.get< Teuchos::RCP<PHX::DataLayout> >("Node Scalar Data Layout");
    std::vector<PHX::DataLayout::size_type> ndims;
    node_dl->dimensions(ndims);
    worksetSize = dims[0];
    numNodes = dims[1];

    // Get data from previous converged time step
    ClatticeName = p.get<std::string>("QP Variable Name")+"_old";
    CLGradName = p.get<std::string>("Gradient QP Variable Name")+"_old";
    eqpsName = p.get<std::string>("eqps Name")+"_old";


    // Allocate workspace for temporary variables
    Hflux.resize(dims[0], numQPs, numDims);
    Hfluxdt.resize(dims[0], numQPs, numDims);
    pterm.resize(dims[0], numQPs);
    tpterm.resize(dims[0], numNodes, numQPs);

    artificalDL.resize(dims[0], numQPs);
    stabilizedDL.resize(dims[0], numQPs);

    C.resize(worksetSize, numQPs, numDims, numDims);
    Cinv.resize(worksetSize, numQPs, numDims, numDims);
    CinvTgrad.resize(worksetSize, numQPs, numDims);
    CinvTgrad_old.resize(worksetSize, numQPs, numDims);
    CinvTaugrad.resize(worksetSize, numQPs, numDims);

    pTTterm.resize(dims[0], numQPs, numDims);
    pBterm.resize(dims[0], numNodes, numQPs, numDims);
    pTranTerm.resize(worksetSize, numDims);

    this->setName("HDiffusionDeformationMatterResidual"+PHX::TypeString<EvalT>::value);

  }

  //**********************************************************************
  template<typename EvalT, typename Traits>
  void HDiffusionDeformationMatterResidual<EvalT, Traits>::
  postRegistrationSetup(typename Traits::SetupData d,
                        PHX::FieldManager<Traits>& fm)
  {
    this->utils.setFieldData(stabParameter,fm);
    this->utils.setFieldData(elementLength,fm);
    this->utils.setFieldData(w_bf_,fm);
    this->utils.setFieldData(w_grad_bf_,fm);
    this->utils.setFieldData(Dstar,fm);
    this->utils.setFieldData(DL,fm);
    this->utils.setFieldData(Clattice,fm);
    this->utils.setFieldData(eqps,fm);
    this->utils.setFieldData(eqpsFactor,fm);
    this->utils.setFieldData(Ctrapped,fm);
    this->utils.setFieldData(Ntrapped,fm);
    this->utils.setFieldData(CLGrad,fm);
    this->utils.setFieldData(stressGrad,fm);
    this->utils.setFieldData(DefGrad,fm);
    this->utils.setFieldData(Pstress,fm);
    this->utils.setFieldData(tauFactor,fm);
    this->utils.setFieldData(weights,fm);
    this->utils.setFieldData(deltaTime,fm);

    //    if (haveSource) this->utils.setFieldData(Source);
    //   if (haveMechSource) this->utils.setFieldData(MechSource);

    this->utils.setFieldData(residual_,fm);
  }

  //**********************************************************************
  template<typename EvalT, typename Traits>
  void HDiffusionDeformationMatterResidual<EvalT, Traits>::
  evaluateFields(typename Traits::EvalData workset)
  {
    typedef Intrepid::FunctionSpaceTools FST;


    Albany::MDArray Clattice_old = (*workset.stateArrayPtr)[ClatticeName];
    Albany::MDArray eqps_old = (*workset.stateArrayPtr)[eqpsName];
    Albany::MDArray CLGrad_old = (*workset.stateArrayPtr)[CLGradName];



    ScalarT dt = deltaTime(0);
    ScalarT temp(1.0);

    ScalarT fac;
    if (dt==0) {
      fac = 1.0e20;
    }
    else
    {
      fac = 1.0/dt;
    }


    // compute artifical diffusivity

    // for 1D this is identical to lumped mass as shown in Prevost's paper.

    for (std::size_t cell=0; cell < workset.numCells; ++cell) {

      for (std::size_t qp=0; qp < numQPs; ++qp) {

        temp = elementLength(cell,qp)*elementLength(cell,qp)/6.0*Dstar(cell,qp)/DL(cell,qp)*fac;

        //                    temp = elementLength(cell,qp)/6.0*Dstar(cell,qp)/DL(cell,qp)*fac - 1/elementLength(cell,qp);
        //                    if (  temp > 1.0 )
        //     {
        artificalDL(cell,qp) = stabParameter(cell,qp)*
          //               (temp) // temp - DL is closer to the limit ...if lumped mass is preferred..
          std::abs(temp) // should be 1 but use 0.5 for safety
          *(0.5 + 0.5*std::tanh( (temp-1)/DL(cell,qp)  ))
          // smoothened Heavside function
          *DL(cell,qp) //*stabParameter(cell,qp)
          ;
        //     }
        /*                    else
                              {
                              artificalDL(cell,qp) =
                              (temp) // 1.25 = safety factor
                              *DL(cell,qp) //*stabParameter(cell,qp)
                              ;
                              }
        */
        //                    cout << temp << endl;
      }

    }

    for (std::size_t cell=0; cell < workset.numCells; ++cell) {

      for (std::size_t qp=0; qp < numQPs; ++qp) {
        stabilizedDL(cell,qp) = artificalDL(cell,qp)/( DL(cell,qp) + artificalDL(cell,qp) );
      }
    }


    // compute the 'material' flux
    FST::tensorMultiplyDataData<ScalarT> (C, DefGrad, DefGrad, 'T');
    Intrepid::RealSpaceTools<ScalarT>::inverse(Cinv, C);
    FST::tensorMultiplyDataData<ScalarT> (CinvTgrad_old, Cinv, CLGrad_old);
    FST::tensorMultiplyDataData<ScalarT> (CinvTgrad, Cinv, CLGrad);

    for (std::size_t cell=0; cell < workset.numCells; ++cell) {

      for (std::size_t qp=0; qp < numQPs; ++qp) {
        for (std::size_t j=0; j<numDims; j++){
          //  CinvTgrad_old(cell,qp,j) = 0.0;
          //   for (std::size_t node=0; node < numNodes; ++node) {
          Hflux(cell,qp,j) = CinvTgrad(cell,qp,j)
            -stabilizedDL(cell,qp)
            *CinvTgrad_old(cell,qp,j)
            //                    }
            ;
        }

      }
    }



    // FST::scalarMultiplyDataData<ScalarT> (Hflux, DL, CLGrad);

    // For debug only
    // FST::integrate<ScalarT>(residual_, CLGrad, w_grad_bf_, Intrepid::COMP_CPP, false); // this one works
    FST::integrate<ScalarT>(residual_, Hflux, w_grad_bf_, Intrepid::COMP_CPP, false); // this also works
    //FST::integrate<ScalarT>(residual_, Hflux, w_grad_bf_, Intrepid::COMP_CPP, false);

    // multiplied the equation by dt.

    for (std::size_t cell=0; cell < workset.numCells; ++cell) {

      for (std::size_t node=0; node < numNodes; ++node) {

        residual_(cell,node) = residual_(cell,node)*dt;
      }
    }






    for (std::size_t cell=0; cell < workset.numCells; ++cell) {

      for (std::size_t node=0; node < numNodes; ++node) {
        //                residual_(cell,node)=0.0;
        for (std::size_t qp=0; qp < numQPs; ++qp) {

          // Transient Term
          residual_(cell,node) +=
            Dstar(cell, qp)/ ( DL(cell,qp)  + artificalDL(cell,qp)  )*
            (Clattice(cell,qp)- Clattice_old(cell, qp) )*
            w_bf_(cell, node, qp);

          // Transient Term
          //residual_(cell,node) += Dstar(cell, qp)*(
          //                 Clattice(cell,qp)- Clattice_old(cell, qp)
          //        )*w_bf_(cell, node, qp)
          //        /DL(cell,qp);

          // Strain Rate Term
          residual_(cell,node) += Ctrapped(cell, qp)/Ntrapped(cell, qp)*
            eqpsFactor(cell,qp)*(
                                 eqps(cell,qp)- eqps_old(cell, qp)
                                 ) *w_bf_(cell, node, qp)
            /(DL(cell,qp) + artificalDL(cell,qp) ) ;

        }
      }
    }




    // hydrostatic stress term
    for (std::size_t cell=0; cell < workset.numCells; ++cell)
    {
      for (std::size_t qp=0; qp < numQPs; ++qp)
      {
        {
          for (std::size_t node=0; node < numNodes; ++node)
          {
            for (std::size_t i=0; i<numDims; i++)
            {
              for (std::size_t j=0; j<numDims; j++)
              {
                residual_(cell,node) -= tauFactor(cell,qp)*
                  w_grad_bf_(cell, node, qp, i)*
                  Cinv(cell,qp,i,j)*
                  stressGrad(cell, qp, j)*dt
                  /( DL(cell,qp) + artificalDL(cell,qp) );
              }

            }
          }
        }
      }
    }



    //---------------------------------------------------------------------------//
    // Stabilization Term


    ScalarT CLPbar(0);
    ScalarT vol(0);

    for (std::size_t cell=0; cell < workset.numCells; ++cell){



      CLPbar = 0.0;
      vol = 0.0;

      for (std::size_t qp=0; qp < numQPs; ++qp) {
        CLPbar += weights(cell,qp)*(
                                    Clattice(cell,qp) - Clattice_old(cell, qp)
                                    );
        vol  += weights(cell,qp);
      }
      CLPbar /= vol;

      for (std::size_t qp=0; qp < numQPs; ++qp) {
        pterm(cell,qp) = CLPbar;
      }

      for (std::size_t node=0; node < numNodes; ++node) {
        trialPbar = 0.0;
        for (std::size_t qp=0; qp < numQPs; ++qp) {
          trialPbar += w_bf_(cell,node,qp);
        }
        trialPbar /= vol;
        for (std::size_t qp=0; qp < numQPs; ++qp) {
          tpterm(cell,node,qp) = trialPbar;
        }

      }

    }

    for (std::size_t cell=0; cell < workset.numCells; ++cell) {
      for (std::size_t node=0; node < numNodes; ++node) {
        for (std::size_t qp=0; qp < numQPs; ++qp) {
          residual_(cell,node) -=
            stabParameter(cell,qp)
            *Dstar(cell, qp)/ ( DL(cell,qp)  + artificalDL(cell,qp)  )*
            (
             - Clattice(cell,qp) + Clattice_old(cell, qp)
             +pterm(cell,qp)
             )
            *(w_bf_(cell, node, qp));

        }
      }
    }
  }
  //**********************************************************************
}

