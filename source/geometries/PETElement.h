// ----------------------------------------------------------------------------
// nexus | PETElement.h
//

#ifndef PETElement_H
#define PETElement_H

#include "GeometryBase.h"

class G4GenericMessenger;

namespace nexus {

  class PETElement: public GeometryBase {
  public:
    /// Constructor
    PETElement(G4String material, G4double width, G4double length);
    G4ThreeVector GetDimensions() const;
    /// Destructor
    ~PETElement();

    void Construct();


  private:

    // Dimension of the crystals
    G4ThreeVector dimensions_;
    G4String crystal_material_;
    G4double crystal_width_;
    G4double crystal_length_;
  };
}
#endif
