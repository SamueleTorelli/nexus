// ----------------------------------------------------------------------------
// nexus | GammaInteractionSteppingAction.cc
//
// This class is a stepping action that records interactions of gamma particles.
// For each gamma interaction, it stores detailed information about the step.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#include "GammaInteractionSteppingAction.h"

#include "FactoryBase.h"

#include <G4Step.hh>
#include <G4Track.hh>
#include <G4ParticleDefinition.hh>
#include <G4RunManager.hh>
#include <G4Event.hh>
#include <G4ProcessManager.hh>
#include <G4VProcess.hh>

using namespace nexus;

REGISTER_CLASS(GammaInteractionSteppingAction, G4UserSteppingAction)

GammaInteractionSteppingAction::GammaInteractionSteppingAction() : G4UserSteppingAction()
{
}

GammaInteractionSteppingAction::~GammaInteractionSteppingAction()
{
}

void GammaInteractionSteppingAction::UserSteppingAction(const G4Step* step)
{
  G4Track* track = step->GetTrack();

  // Only process gamma particles
  if (track->GetDefinition()->GetParticleName() != "gamma") return;

  // Get the process name for the step
  G4String proc_name = "Transportation";  // Default if no process
  if (step->GetPostStepPoint()->GetProcessDefinedStep()) {
    proc_name = step->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName();
  }

  // Skip transportation steps (no interaction)
  if (proc_name == "Transportation") return;

  // Create a new interaction record
  GammaInteraction inter;

  // Event and particle information
  inter.event_id = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
  inter.particle_id = track->GetTrackID();
  inter.particle_name = track->GetDefinition()->GetParticleName();
  inter.primary = (track->GetParentID() == 0);
  inter.mother_id = track->GetParentID();

  // Initial (pre-step) position and time
  G4ThreeVector initial_pos = step->GetPreStepPoint()->GetPosition();
  inter.initial_x = initial_pos.x();
  inter.initial_y = initial_pos.y();
  inter.initial_z = initial_pos.z();
  inter.initial_t = step->GetPreStepPoint()->GetGlobalTime();

  // Final (post-step) position and time
  G4ThreeVector final_pos = step->GetPostStepPoint()->GetPosition();
  inter.final_x = final_pos.x();
  inter.final_y = final_pos.y();
  inter.final_z = final_pos.z();
  inter.final_t = step->GetPostStepPoint()->GetGlobalTime();

  // Volumes
  inter.initial_volume = "";
  inter.final_volume = "";
  if (step->GetPreStepPoint()->GetPhysicalVolume())
    inter.initial_volume = step->GetPreStepPoint()->GetPhysicalVolume()->GetName();
  if (step->GetPostStepPoint()->GetPhysicalVolume())
    inter.final_volume = step->GetPostStepPoint()->GetPhysicalVolume()->GetName();

  // Initial momentum
  G4ThreeVector initial_mom = step->GetPreStepPoint()->GetMomentum();
  inter.initial_momentum_x = initial_mom.x();
  inter.initial_momentum_y = initial_mom.y();
  inter.initial_momentum_z = initial_mom.z();

  // Final momentum
  G4ThreeVector final_mom = step->GetPostStepPoint()->GetMomentum();
  inter.final_momentum_x = final_mom.x();
  inter.final_momentum_y = final_mom.y();
  inter.final_momentum_z = final_mom.z();

  // Kinetic energy (pre-step)
  inter.kin_energy = step->GetPreStepPoint()->GetKineticEnergy();

  // Step length
  inter.length = step->GetStepLength();

  // Creator process
  inter.creator_proc = "";
  if (track->GetCreatorProcess()) {
    inter.creator_proc = track->GetCreatorProcess()->GetProcessName();
  }

  // Final process
  inter.final_proc = proc_name;

  // Store the interaction
  interactions_.push_back(inter);
}

void GammaInteractionSteppingAction::Reset()
{
  interactions_.clear();
}