// ----------------------------------------------------------------------------
// petalosim | NEMASensitivity.h
//
// This class implements the geometry of a NEMA Sensitivity phantom.
//
// The CRYSP Collaboration
// ----------------------------------------------------------------------------

#ifndef NEMA_SENSITIVITY_H
#define NEMA_SENSITIVITY_H

#include "GeometryBase.h"

class G4Material;
class G4GenericMessenger;
class G4Navigator;

namespace nexus
{
  class CylinderPointSampler2020;
}

using namespace nexus;

class NEMASensitivity: public GeometryBase
{

 public:
  NEMASensitivity();
  ~NEMASensitivity();

  void Construct();

  G4ThreeVector GenerateVertex(const G4String &/*region*/) const;

 private:

  CylinderPointSampler2020* cyl_gen_;
};

#endif
