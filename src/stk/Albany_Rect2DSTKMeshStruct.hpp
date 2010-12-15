/********************************************************************\
*            Albany, Copyright (2010) Sandia Corporation             *
*                                                                    *
* Notice: This computer software was prepared by Sandia Corporation, *
* hereinafter the Contractor, under Contract DE-AC04-94AL85000 with  *
* the Department of Energy (DOE). All rights in the computer software*
* are reserved by DOE on behalf of the United States Government and  *
* the Contractor as provided in the Contract. You are authorized to  *
* use this computer software for Governmental purposes but it is not *
* to be released or distributed to the public. NEITHER THE GOVERNMENT*
* NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR      *
* ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE. This notice    *
* including this sentence must appear on any copies of this software.*
*    Questions to Andy Salinger, agsalin@sandia.gov                  *
\********************************************************************/


#ifndef ALBANY_RECT2DSTKMESHSTRUCT_HPP
#define ALBANY_RECT2DSTKMESHSTRUCT_HPP

#include "Albany_AbstractSTKMeshStruct.hpp"


namespace Albany {

  struct Rect2DSTKMeshStruct : public AbstractSTKMeshStruct {

    Rect2DSTKMeshStruct(
                  const Teuchos::RCP<const Epetra_Comm>& epetra_comm,
                  const Teuchos::RCP<Teuchos::ParameterList>& params,
                  const unsigned int neq_);

    ~Rect2DSTKMeshStruct();

    Teuchos::RCP<const Teuchos::ParameterList>
      getValidDiscretizationParameters() const;

    bool periodic;
    bool triangles; // Deaults to false, meaning quad elements

  };

}

#endif