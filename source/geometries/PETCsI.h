// ----------------------------------------------------------------------------
// nexus | PETCsI.h
//

#ifndef PETCsI_H
#define PETCsI_H

#include "GeometryBase.h"
#include "CylinderPointSampler2020.h"
#include "SpherePointSampler.h"
#include "BoxPointSampler.h"
#include "JaszczakPhantom.h"
#include "NEMASensitivity.h"
#include "NEMANECR.h"

class G4GenericMessenger;

namespace nexus {

  class PETCsI: public GeometryBase {
  public:
    /// Constructor
    PETCsI();

    /// Destructor
    ~PETCsI();

    void Construct();
    G4ThreeVector GenerateVertex(const G4String& region) const;


  private:

    // Messenger for the definition of control commands
    G4GenericMessenger* msg_;
    CylinderPointSampler2020* cylindrical_shell_;
    JaszczakPhantom* jas_phantom_;
    NEMASensitivity* nema_sensitivity_;
    NEMANECR* nema_necr_;

    // Dimension of the crystals
    G4String crystal_material_;
    G4double crystal_width_;
    G4double crystal_length_;
    G4double pet_diameter_;
    G4double pet_length_;
    G4String phantom_;
    G4bool mixed_;
  };
}
#endif
