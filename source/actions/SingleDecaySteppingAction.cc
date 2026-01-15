// ----------------------------------------------------------------------------
// nexus | SingleDecaySteppingAction.cc
//
// This class allows the user to print the total number of photons detected by
// all kinds of photosensors at the end of the run.
// It also shows examples of information that can be accessed at the stepping
// level, so it is useful for debugging.
//
// The  NEXT Collaboration
// ----------------------------------------------------------------------------

#include "SingleDecaySteppingAction.h"
#include "FactoryBase.h"

#include <G4Step.hh>
#include <G4SteppingManager.hh>
#include <G4ProcessManager.hh>
#include <G4OpticalPhoton.hh>
#include <G4OpBoundaryProcess.hh>
#include <G4VPhysicalVolume.hh>

using namespace nexus;

REGISTER_CLASS(SingleDecaySteppingAction, G4UserSteppingAction)

SingleDecaySteppingAction::SingleDecaySteppingAction():
  G4UserSteppingAction(),
  msg_(nullptr),
  z_(-9),
  a_(-9)
{
  msg_ = new G4GenericMessenger(this, "/Actions/SingleDecaySteppingAction/",
                                "Control commands for SingleDecaySteppingAction");

  G4GenericMessenger::Command& define_single_cmd =
    msg_->DeclareMethod("DefineSingle",
                        &SingleDecaySteppingAction::DefineSingle,
                        "Set the mass number (A) and atomic number (Z) of the ion to keep");
  define_single_cmd.SetParameterName("A", false);
  define_single_cmd.SetParameterName("Z", false);
}

void SingleDecaySteppingAction::DefineSingle(G4int a, G4int z)
{
  a_ = a;
  z_ = z;
}

SingleDecaySteppingAction::~SingleDecaySteppingAction()
{
  delete msg_;
  msg_ = nullptr;

  G4double total_counts = 0;
  detectorCounts::iterator it = my_counts_.begin();
  while (it != my_counts_.end()) {
    G4cout << "Detector " << it->first << ": " << it->second << " counts" << G4endl;
    total_counts += it->second;
    it ++;
  }
  G4cout << "TOTAL COUNTS: " << total_counts << G4endl;
}


void SingleDecaySteppingAction::UserSteppingAction(const G4Step* step)
{
  G4ParticleDefinition* pdef = step->GetTrack()->GetDefinition();

  if (pdef->GetParticleType() == "nucleus") {
    // The particle is an ion
    // Check if its atomic number or mass number do not match the configured (a,z)
    G4int A = pdef->GetAtomicMass();
    G4int Z = pdef->GetAtomicNumber();
    if (A != a_ || Z != z_ ) {
      step->GetTrack()->SetTrackStatus(fStopAndKill);
      return;
    }
  }

  //Check whether the track is an optical photon
  if (pdef != G4OpticalPhoton::Definition()) return;

  // Retrieve the pointer to the optical boundary process.
  // We do this only once per run defining our local pointer as static.
  static G4OpBoundaryProcess* boundary = 0;

  if (!boundary) { // the pointer is not defined yet
    // Get the list of processes defined for the optical photon
    // and loop through it to find the optical boundary process.
    G4ProcessVector* pv = pdef->GetProcessManager()->GetProcessList();
    for (size_t i=0; i<pv->size(); i++) {
      if ((*pv)[i]->GetProcessName() == "OpBoundary") {
	boundary = (G4OpBoundaryProcess*) (*pv)[i];
	break;
      }
    }
  }

  if (step->GetPostStepPoint()->GetStepStatus() == fGeomBoundary) {
    if (boundary->GetStatus() == Detection ){
      G4String detector_name = step->GetPostStepPoint()->GetTouchableHandle()->GetVolume()->GetName();
      //G4cout << "##### Sensitive Volume: " << detector_name << G4endl;

      detectorCounts::iterator it = my_counts_.find(detector_name);
      if (it != my_counts_.end()) my_counts_[it->first] += 1;
      else my_counts_[detector_name] = 1;
    }
  }

  return;
}
