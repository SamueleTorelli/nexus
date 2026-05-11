#include "SensorTriggeredEventAction.h"

#include "PersistencyManager.h"
#include "SensorSD.h"
#include "SensorHit.h"
#include "FactoryBase.h"

#include <G4HCofThisEvent.hh>
#include <G4HCtable.hh>
#include <G4SDManager.hh>
#include <G4VPersistencyManager.hh>
#include <G4VHitsCollection.hh>
#include <G4Event.hh>

using namespace nexus;

REGISTER_CLASS(SensorTriggeredEventAction, G4UserEventAction)

SensorTriggeredEventAction::SensorTriggeredEventAction()
{
  // In this mode the definition of an "interesting" event is based on sensor
  // (SiPM) hits instead of deposited ionization energy.
  PersistencyManager* pm = dynamic_cast<PersistencyManager*>(
    G4VPersistencyManager::GetPersistencyManager());
  if (pm) {
    pm->SaveNumbOfInteractingEvents(true);
  }
}

SensorTriggeredEventAction::~SensorTriggeredEventAction() = default;

void SensorTriggeredEventAction::EndOfEventAction(const G4Event* event)
{
  if (!event) return;

  auto* pm = dynamic_cast<PersistencyManager*>(
    G4VPersistencyManager::GetPersistencyManager());
  if (!pm) return;

  bool store_evt = false;

  if (!event->IsAborted()) {
    G4HCofThisEvent* hce = event->GetHCofThisEvent();
    if (hce) {
      G4SDManager* sdmgr = G4SDManager::GetSDMpointer();
      G4HCtable* hct = sdmgr->GetHCtable();
      if (hct) {
        const auto sensor_hc_unique = SensorSD::GetCollectionUniqueName();
        for (size_t i = 0; i < hct->entries(); i++) {
          const G4String hcname = hct->GetHCname(i);
          if (hcname != sensor_hc_unique) continue;

          const G4String sdname = hct->GetSDname(i);
          const G4int hcid = sdmgr->GetCollectionID(sdname + "/" + hcname);
          if (hcid < 0) continue;

          auto* hc = hce->GetHC(hcid);
          auto* sensorHits = dynamic_cast<SensorHitsCollection*>(hc);
          if (sensorHits && sensorHits->entries() > 0) {
            store_evt = true;
            break;
          }
        }
      }
    }
  }

  pm->InteractingEvent(store_evt);
  pm->StoreCurrentEvent(store_evt);
}

