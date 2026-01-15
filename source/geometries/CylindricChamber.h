// ----------------------------------------------------------------------------
// nexus | CylindricChamber.h
//
// General-purpose cylindric chamber.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#ifndef CYLINDRIC_CHAMBER_H
#define CYLINDRIC_CHAMBER_H

#include "GeometryBase.h"

class G4GenericMessenger;


namespace nexus {

  class CylindricChamber: public GeometryBase
  {
  public:
    /// Constructor
    CylindricChamber();
    /// Destructor
    ~CylindricChamber();

    /// Return vertex within region <region> of the chamber
    virtual G4ThreeVector GenerateVertex(const G4String& region) const;

    virtual void Construct();

  private:
    /// Messenger for the definition of control commands
    G4GenericMessenger* msg_;
    G4String target_;
    G4double sc_yield_, e_lifetime_;
    G4double pressure_, temperature_;
    // Chamber geometry parameters
    G4double chamber_diam_;
    G4double chamber_length_;
    G4double chamber_thickn_;
    G4double step_max_;
  };

} // end namespace nexus

#endif
