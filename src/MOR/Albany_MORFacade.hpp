//*****************************************************************//
//    Albany 2.0:  Copyright 2012 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//

#ifndef ALBANY_MORFACADE_HPP
#define ALBANY_MORFACADE_HPP

#include "Teuchos_ParameterList.hpp"
#include "Teuchos_RCP.hpp"

namespace Albany {

class AbstractDiscretization;

class ReducedOrderModelFactory;
class MORObserverFactory;

class MORFacade {
public:
  virtual Teuchos::RCP<ReducedOrderModelFactory> modelFactory() const = 0;
  virtual Teuchos::RCP<MORObserverFactory> observerFactory() const = 0;

  virtual ~MORFacade() {}
};

// Entry point
Teuchos::RCP<MORFacade> createMORFacade(
    const Teuchos::RCP<AbstractDiscretization> &disc,
    const Teuchos::RCP<Teuchos::ParameterList> &params);

}

#endif /* ALBANY_MORFACADE_HPP */