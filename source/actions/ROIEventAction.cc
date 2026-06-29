// ----------------------------------------------------------------------------
// nexus | ROIEventAction.cc
//
// Event action that counts every event but only saves events whose primary
// positron-pair energy sum exceeds a configurable threshold.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#include "ROIEventAction.h"
#include "PersistencyManager.h"
#include "FactoryBase.h"

#include <G4Event.hh>
#include <G4GenericMessenger.hh>
#include <G4PrimaryVertex.hh>
#include <G4PrimaryParticle.hh>
#include <G4VPersistencyManager.hh>
#include <globals.hh>

namespace nexus {

REGISTER_CLASS(ROIEventAction, G4UserEventAction)

ROIEventAction::ROIEventAction():
  G4UserEventAction(), nevt_(0), nupdate_(10), positron_energy_threshold_(2.5)
{
  msg_ = new G4GenericMessenger(this, "/Actions/ROIEventAction/");

  G4GenericMessenger::Command& thresh_cmd =
    msg_->DeclareProperty("positron_energy_threshold", positron_energy_threshold_,
                          "Minimum sum of primary positron energies to save the event.");
  thresh_cmd.SetParameterName("positron_energy_threshold", true);
  thresh_cmd.SetUnitCategory("Energy");
  thresh_cmd.SetRange("positron_energy_threshold>=0.");
}

ROIEventAction::~ROIEventAction()
{
}

void ROIEventAction::BeginOfEventAction(const G4Event* /*event*/)
{
  if ((nevt_ % nupdate_) == 0) {
    G4cout << " >> Event no. " << nevt_ << G4endl;
    if (nevt_ == (10 * nupdate_)) nupdate_ *= 10;
  }
}

void ROIEventAction::EndOfEventAction(const G4Event* event)
{
  nevt_++;

  G4double positron_energy_sum = 0.;
  if (event) {
    for (G4int iv = 0; iv < event->GetNumberOfPrimaryVertex(); ++iv) {
      const G4PrimaryVertex* vertex = event->GetPrimaryVertex(iv);
      if (!vertex) continue;

      for (G4int ip = 0; ip < vertex->GetNumberOfParticle(); ++ip) {
        const G4PrimaryParticle* particle = vertex->GetPrimary(ip);
        if (!particle) continue;

        if (particle->GetPDGcode() == -11) {
          positron_energy_sum += particle->GetKineticEnergy();
        }
      }
    }
  }

  PersistencyManager* pm = dynamic_cast<PersistencyManager*>(
    G4VPersistencyManager::GetPersistencyManager());

  if (!event->IsAborted()) {
    pm->InteractingEvent(true);
  } else {
    pm->InteractingEvent(false);
  }

  if (!event->IsAborted() && positron_energy_sum > positron_energy_threshold_ * MeV) {
    pm->StoreCurrentEvent(true);
  } else {
    pm->StoreCurrentEvent(false);
  }
}

} // namespace nexus
