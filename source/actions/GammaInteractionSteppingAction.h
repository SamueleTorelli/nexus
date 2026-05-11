// ----------------------------------------------------------------------------
// nexus | GammaInteractionSteppingAction.h
//
// This class is a stepping action that records interactions of gamma particles.
// For each gamma interaction, it stores detailed information about the step.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#ifndef GAMMA_INTERACTION_STEPPING_ACTION_H
#define GAMMA_INTERACTION_STEPPING_ACTION_H

#include <G4UserSteppingAction.hh>
#include <G4Types.hh>

#include <vector>
#include <string>

namespace nexus {

struct GammaInteraction {
  G4int event_id;
  G4int particle_id;
  std::string particle_name;
  G4bool primary;
  G4int mother_id;
  G4double initial_x, initial_y, initial_z, initial_t;
  G4double final_x, final_y, final_z, final_t;
  std::string initial_volume, final_volume;
  G4double initial_momentum_x, initial_momentum_y, initial_momentum_z;
  G4double final_momentum_x, final_momentum_y, final_momentum_z;
  G4double kin_energy;
  G4double length;
  std::string creator_proc;
  std::string final_proc;
};

class GammaInteractionSteppingAction : public G4UserSteppingAction {
public:
  GammaInteractionSteppingAction();
  ~GammaInteractionSteppingAction();

  void UserSteppingAction(const G4Step* step);

  const std::vector<GammaInteraction>& GetInteractions() const { return interactions_; }
  void Reset();

private:
  std::vector<GammaInteraction> interactions_;
};

} // namespace nexus

#endif