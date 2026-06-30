// ----------------------------------------------------------------------------
// nexus | Decay0DoublePositronInterface.h
//
// This class is a primary generator for a Xe-136 double-beta-like process,
// interfacing the DECAY0 C++ code and converting the generated electrons to
// positrons while preserving the original decay spectrum.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#ifndef DECAY0_DOUBLE_POSITRON_INTERFACE_H
#define DECAY0_DOUBLE_POSITRON_INTERFACE_H

#include <G4VPrimaryGenerator.hh>
#include <fstream>

class G4GenericMessenger;
class G4Event;
class G4PrimaryParticle;
#include "decay0.h"

namespace nexus {

  class GeometryBase;

  class Decay0DoublePositronInterface : public G4VPrimaryGenerator
  {
  public:
    Decay0DoublePositronInterface();
    ~Decay0DoublePositronInterface();

    void GeneratePrimaryVertex(G4Event*);

  private:
    G4GenericMessenger* msg_;

    G4String decay_file_;
    G4String region_;

    decay0* decay0_;
    int Xe136DecayMode_;
    int Ba136FinalState_;

    double qValueMeV_;
    double energyThreshold_;

    std::ofstream fOutDebug_;
    const GeometryBase* geom_;

    inline void SetEnergyThreshold(double e) { energyThreshold_ = e; }
    inline void SetXe136DecayMode(int dcm) { Xe136DecayMode_ = dcm; }
    inline void SetBa136FinalState(int fs) { Ba136FinalState_ = fs; }
    inline void SetQValue(double q) {
      qValueMeV_ = q;
      if (decay0_ != 0 && q > 0.) decay0_->SetQValue(q);
    }
  };

} // end namespace nexus

#endif
