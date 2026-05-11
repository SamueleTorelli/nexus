// ----------------------------------------------------------------------------
// nexus | PersistencyManager.cc
//
// This class writes all the relevant information of the simulation
// to an ouput file.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#include "PersistencyManager.h"

#include "Trajectory.h"
#include "TrajectoryMap.h"
#include "IonizationSD.h"
#include "SensorSD.h"
#include "NexusApp.h"
#include "DetectorConstruction.h"
#include "SaveAllSteppingAction.h"
#include "GammaInteractionSteppingAction.h"
#include "GeometryBase.h"
#include "HDF5Writer.h"
#include "PersistencyManagerBase.h"
#include "FactoryBase.h"

#include <G4GenericMessenger.hh>
#include <G4Event.hh>
#include <G4TrajectoryContainer.hh>
#include <G4Trajectory.hh>
#include <G4SDManager.hh>
#include <G4HCtable.hh>
#include <G4RunManager.hh>
#include <G4Run.hh>

#include <algorithm>
#include <cctype>
#include <string>
#include <sstream>
#include <iostream>
#include <string>

using namespace nexus;


REGISTER_CLASS(PersistencyManager, PersistencyManagerBase)


PersistencyManager::PersistencyManager():
PersistencyManagerBase(), msg_(0), output_file_("nexus_out"), ready_(false),
  store_evt_(true), store_steps_(false),
  interacting_evt_(false), save_ie_numb_(false), event_type_("other"),
  saved_evts_(0), interacting_evts_(0), pmt_bin_size_(-1), sipm_bin_size_(-1),
  nevt_(0), start_id_(0), first_evt_(true), h5writer_(0)
{
  msg_ = new G4GenericMessenger(this, "/nexus/persistency/");
  msg_->DeclareProperty("output_file", output_file_, "Path of output file.");
  msg_->DeclareProperty("event_type", event_type_,
                        "Type of event: bb0nu, bb2nu, background.");
  msg_->DeclareProperty("start_id", start_id_,
                        "Starting event ID for this job.");

  init_macro_ = "";
  macros_.clear();
  delayed_macros_.clear();
  secondary_macros_.clear();
}



PersistencyManager::~PersistencyManager()
{
  delete msg_;
  delete h5writer_;
}



void PersistencyManager::OpenFile()
{
  // If the output file was not set yet, do so
  if (!h5writer_) {
    h5writer_ = new HDF5Writer();
    G4String hdf5file = GetUniqueOutputFileName();
    h5writer_->Open(hdf5file, store_steps_);
    return;
  } else {
    G4Exception("[PersistencyManager]", "OpenFile()",
		JustWarning, "An output file was previously opened.");
  }
}



G4String PersistencyManager::GetUniqueOutputFileName() const
{
  const G4String base = output_file_;
  G4String candidate = base + ".h5";

  std::ifstream file(candidate.c_str(), std::ifstream::in);
  if (!file.good())
    return candidate;
  file.close();

  for (int suffix = 1; ; ++suffix) {
    candidate = base + "_" + std::to_string(suffix) + ".h5";
    std::ifstream numbered(candidate.c_str(), std::ifstream::in);
    if (!numbered.good())
      return candidate;
    numbered.close();
  }
}



void PersistencyManager::CloseFile()
{
  if (!h5writer_) return;

  h5writer_->Close();
}



G4bool PersistencyManager::Store(const G4Event* event)
{
  if (interacting_evt_) {
    interacting_evts_++;
  }

  // Initialize the event counter once per run.
  // Note: `nevt_` is used as the `event_id` written in the `/MC/particles` table.
  if (first_evt_) {
    first_evt_ = false;
    nevt_ = start_id_;
  }

  // Always store MC/particles (trajectories), even if we don't store hits.
  // This decouples the particle output from the sensor-trigger selection.
  if (store_evt_) {
    saved_evts_++;

    if (store_steps_)
      StoreSteps();
  } else {
    // If we are not storing the event payload, make sure we don't keep
    // stepping information around (prevents unbounded growth).
    if (store_steps_) {
      SaveAllSteppingAction* sa = (SaveAllSteppingAction*)
        G4RunManager::GetRunManager()->GetUserSteppingAction();
      sa->Reset();
    }
  }

  // Store the trajectories of the event (MC/particles table)
  StoreTrajectories(event->GetTrajectoryContainer());

  // Store gamma interactions from the stepping action
  StoreGammaInteractions();

  // Store ionization hits and sensor hits only when the event is selected.
  if (store_evt_) {
    // Clear per-event hit association cache.
    ihits_ = nullptr;
    hit_map_.clear();

    StoreHits(event->GetHCofThisEvent());
    G4cout << "EVENTID" << nevt_ << " " << total_energy_ << " " << total_energy_csi_ << " " << total_energy_bgo_ << G4endl;
    total_energy_bgo_ = 0;
    total_energy_csi_ = 0;
    total_energy_ = 0;

    // Keep the previous behavior: mark current event as stored.
    StoreCurrentEvent(true);
  }

  // Release trajectories cached in memory for the next event.
  TrajectoryMap::Clear();

  // Increment event id for every event so `/MC/particles` always has
  // a consistent mapping to Geant4 event sequence numbers.
  nevt_++;

  return store_evt_;
}


void PersistencyManager::StoreTrajectories(G4TrajectoryContainer* tc)
{
  // If the pointer is null, no trajectories were stored in this event
  if (!tc) return;

  // Store a subset of trajectories in the `/MC/particles` table.
  //
  // Requested behavior:
  // - track all primary gammas (primary photon trajectories)
  // - also track gammas produced by Compton scattering (secondary photons)
  //
  // Fallback behavior: if no relevant photons are found, keep the previous
  // "store primaries" logic to avoid an empty `/MC/particles` table.
  std::vector<Trajectory*> primaries;
  primaries.reserve(tc->entries());

  std::vector<Trajectory*> photons_to_store;
  photons_to_store.reserve(tc->entries());

  for (size_t i = 0; i < tc->entries(); ++i) {
    Trajectory* trj = dynamic_cast<Trajectory*>((*tc)[i]);
    if (!trj) continue;

    // Used for fallback if no selected photons are found.
    if (trj->GetParentID() == 0) primaries.push_back(trj);

    // Select photons only.
    // In Geant4, photons are typically PDG 22.
    if (trj->GetPDGEncoding() != 22) continue;

    const bool is_primary_gamma = (trj->GetParentID() == 0);

    // For secondary photons, the "creator process" is (typically) "compt"
    // when produced by Compton scattering.
    std::string creator_proc = trj->GetCreatorProcess();
    std::transform(creator_proc.begin(), creator_proc.end(), creator_proc.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    const bool is_compton_gamma = (creator_proc.find("compt") != std::string::npos);

    if (is_primary_gamma || is_compton_gamma) {
      photons_to_store.push_back(trj);
    }
  }

  std::vector<Trajectory*> to_store;

  if (!photons_to_store.empty()) {
    to_store.swap(photons_to_store);
  } else if (!primaries.empty()) {
    to_store.swap(primaries);
  } else {
    // Fallback: if no primaries were recorded, store the first two trajectories
    // to avoid an empty `/MC/particles` table.
    for (size_t i = 0; i < tc->entries(); ++i) {
      Trajectory* trj = dynamic_cast<Trajectory*>((*tc)[i]);
      if (!trj) continue;
      to_store.push_back(trj);
      if (to_store.size() >= 2) break;
    }
  }

  for (auto* trj : to_store) {
    if (!trj) continue;

    const G4int trackid = trj->GetTrackID();

    const char primary = (trj->GetParentID() == 0) ? 1 : 0;
    const G4int mother_id = trj->GetParentID();

    const G4String particle_name = trj->GetParticleName();
    const G4String ini_volume = trj->GetInitialVolume();
    const G4String final_volume = trj->GetFinalVolume();

    const G4double length = trj->GetTrackLength();

    const G4ThreeVector ini_xyz = trj->GetInitialPosition();
    const G4double ini_t = trj->GetInitialTime();

    const G4ThreeVector final_xyz = trj->GetFinalPosition();
    const G4double final_t = trj->GetFinalTime();

    const G4double mass = trj->GetParticleDefinition()->GetPDGMass();
    const G4ThreeVector ini_mom = trj->GetInitialMomentum();
    const G4double energy = sqrt(ini_mom.mag2() + mass * mass);
    const G4ThreeVector final_mom = trj->GetFinalMomentum();
    const float kin_energy = static_cast<float>(energy - mass);

    const G4String creator_proc = trj->GetCreatorProcess();
    const G4String final_proc = trj->GetFinalProcess();

    h5writer_->WriteParticleInfo(nevt_,
                                  trackid,
                                  particle_name.c_str(),
                                  primary,
                                  mother_id,
                                  (float)ini_xyz.x(), (float)ini_xyz.y(), (float)ini_xyz.z(),
                                  (float)ini_t,
                                  (float)final_xyz.x(), (float)final_xyz.y(), (float)final_xyz.z(),
                                  (float)final_t,
                                  ini_volume.c_str(), final_volume.c_str(),
                                  (float)ini_mom.x(), (float)ini_mom.y(), (float)ini_mom.z(),
                                  (float)final_mom.x(), (float)final_mom.y(), (float)final_mom.z(),
                                  kin_energy,
                                  (float)length,
                                  creator_proc.c_str(),
                                  final_proc.c_str());
  }
}



void PersistencyManager::StoreHits(G4HCofThisEvent* hce)
{
  if (!hce) return;

  G4SDManager* sdmgr = G4SDManager::GetSDMpointer();
  G4HCtable* hct = sdmgr->GetHCtable();

  // Loop through the hits collections
  for (auto i=0; i<hct->entries(); i++) {

    // Collection are identified univocally (in principle) using
    // their id number, and this can be obtained using the collection
    // and sensitive detector names.
    G4String hcname = hct->GetHCname(i);
    G4String sdname = hct->GetSDname(i);
    int hcid = sdmgr->GetCollectionID(sdname+"/"+hcname);

    // Fetch collection using the id number
    G4VHitsCollection* hits = hce->GetHC(hcid);

    if (hcname == IonizationSD::GetCollectionUniqueName()) {
      StoreIonizationHits(hits);
    } else if (hcname == SensorSD::GetCollectionUniqueName()) {
      StoreSensorHits(hits);
    } else {
      G4String msg =
        "Collection of hits '" + sdname + "/" + hcname
        + "' is of an unknown type and will not be stored.";
      G4Exception("[PersistencyManager]", "StoreHits()", JustWarning, msg);
    }
  }

}


void PersistencyManager::StoreIonizationHits(G4VHitsCollection* hc)
{
  IonizationHitsCollection* hits =
    dynamic_cast<IonizationHitsCollection*>(hc);
  if (!hits) return;

  std::string sdname = hits->GetSDname();

  // total_energy_ = 0.;
  for (size_t i=0; i<hits->entries(); i++) {
    IonizationHit* hit = dynamic_cast<IonizationHit*>(hits->GetHit(i));
    if (!hit) continue;

    G4int trackid = hit->GetTrackID();


    std::map<G4int, std::vector<G4int>* >::iterator it = hit_map_.find(trackid);
    if (it != hit_map_.end()) {
      ihits_ = it->second;
    } else {
      ihits_ = new std::vector<G4int>;
      hit_map_[trackid] = ihits_;
    }

    ihits_->push_back(1);

    G4ThreeVector xyz = hit->GetPosition();
    // h5writer_->WriteHitInfo(nevt_, trackid,  ihits_->size() - 1,
		// 	    xyz[0], xyz[1], xyz[2],
		// 	    hit->GetTime(), hit->GetEnergyDeposit(),
		// 	    sdname.c_str());
    total_energy_ += hit->GetEnergyDeposit();
    if (sdname == "BGO")
      total_energy_bgo_ += hit->GetEnergyDeposit();
    if (sdname == "CSI")
      total_energy_csi_ += hit->GetEnergyDeposit();
  }
}



void PersistencyManager::StoreSensorHits(G4VHitsCollection* hc)
{
  SensorHitsCollection* hits = dynamic_cast<SensorHitsCollection*>(hc);
  if (!hits) return;

  std::string sdname = hits->GetSDname();

  std::map<G4String, G4double>::const_iterator sensdet_it = sensdet_bin_.find(sdname);
  if (sensdet_it == sensdet_bin_.end()) {
    for (size_t j=0; j<hits->entries(); j++) {
      SensorHit* hit = dynamic_cast<SensorHit*>(hits->GetHit(j));
      if (!hit) continue;
      G4double bin_size = hit->GetBinSize();
      sensdet_bin_[sdname] = bin_size;
      break;
    }
  }

  for (size_t i=0; i<hits->entries(); i++) {

    SensorHit* hit = dynamic_cast<SensorHit*>(hits->GetHit(i));
    if (!hit) continue;

    G4ThreeVector xyz = hit->GetPosition();
    G4double binsize = hit->GetBinSize();

    const std::map<G4double, G4int>& wvfm = hit->GetHistogram();
    std::map<G4double, G4int>::const_iterator it;
    std::vector< std::pair<unsigned int,float> > data;
    G4double amplitude = 0.;

    for (it = wvfm.begin(); it != wvfm.end(); ++it) {
      unsigned int time_bin = (unsigned int)((*it).first/binsize+0.5);
      unsigned int charge = (unsigned int)((*it).second+0.5);

      data.push_back(std::make_pair(time_bin, charge));
      amplitude = amplitude + (*it).second;

      h5writer_->WriteSensorDataInfo(nevt_, (unsigned int)hit->GetPmtID(),
                                     time_bin, charge);
    }

    std::vector<G4int>::iterator pos_it =
      std::find(sns_posvec_.begin(), sns_posvec_.end(), hit->GetPmtID());
    if (pos_it == sns_posvec_.end()) {
      h5writer_->WriteSensorPosInfo((unsigned int)hit->GetPmtID(), sdname.c_str(),
				    (float)xyz.x(), (float)xyz.y(), (float)xyz.z());
      sns_posvec_.push_back(hit->GetPmtID());
    }

  }
}


void PersistencyManager::StoreSteps()
{
  SaveAllSteppingAction* sa = (SaveAllSteppingAction*)
    G4RunManager::GetRunManager()->GetUserSteppingAction();

  StepContainer<G4String> initial_volumes = sa->get_initial_volumes();
  StepContainer<G4String>   final_volumes = sa->get_final_volumes  ();
  StepContainer<G4String>      proc_names = sa->get_proc_names     ();

  StepContainer<G4ThreeVector> initial_poss = sa->get_initial_poss();
  StepContainer<G4ThreeVector>   final_poss = sa->get_final_poss  ();
  StepContainer<G4double>             times = sa->get_times       ();

  for (auto it = initial_volumes.begin(); it != initial_volumes.end(); ++it) {
    std::pair<G4int, G4String> key           = it->first;
    G4int                      track_id      = key.first;
    G4String                   particle_name = key.second;

    for (size_t step_id=0; step_id < it->second.size(); ++step_id) {
      h5writer_->WriteStep(nevt_, track_id, particle_name, step_id,
                           initial_volumes[key][step_id],
                             final_volumes[key][step_id],
                                proc_names[key][step_id],
                           initial_poss   [key][step_id].x(),
                           initial_poss   [key][step_id].y(),
                           initial_poss   [key][step_id].z(),
                             final_poss   [key][step_id].x(),
                             final_poss   [key][step_id].y(),
                             final_poss   [key][step_id].z(),
                                  times   [key][step_id]);
    }
  }
  sa->Reset();
}

void PersistencyManager::StoreGammaInteractions()
{
  const GammaInteractionSteppingAction* gamma_action = dynamic_cast<const GammaInteractionSteppingAction*>(
    G4RunManager::GetRunManager()->GetUserSteppingAction());
  
  // Only process if this stepping action is registered
  if (!gamma_action) return;

  const std::vector<GammaInteraction>& interactions = gamma_action->GetInteractions();
  
  for (const auto& inter : interactions) {
    h5writer_->WriteGammaInteraction(
      inter.event_id,
      inter.particle_id,
      inter.particle_name.c_str(),
      inter.primary ? 1 : 0,
      inter.mother_id,
      inter.initial_x,
      inter.initial_y,
      inter.initial_z,
      inter.initial_t,
      inter.final_x,
      inter.final_y,
      inter.final_z,
      inter.final_t,
      inter.initial_volume.c_str(),
      inter.final_volume.c_str(),
      inter.initial_momentum_x,
      inter.initial_momentum_y,
      inter.initial_momentum_z,
      inter.final_momentum_x,
      inter.final_momentum_y,
      inter.final_momentum_z,
      inter.kin_energy,
      inter.length,
      inter.creator_proc.c_str(),
      inter.final_proc.c_str()
    );
  }
  
  // Cast away const to reset for next event
  GammaInteractionSteppingAction* gamma_action_mutable = 
    const_cast<GammaInteractionSteppingAction*>(gamma_action);
  gamma_action_mutable->Reset();
}

G4bool PersistencyManager::Store(const G4Run*)
{
  // Store the event type
  G4String key = "event_type";
  h5writer_->WriteRunInfo(key, event_type_.c_str());

  // Store the number of events to be processed
  NexusApp* app = (NexusApp*) G4RunManager::GetRunManager();
  G4int num_events = app->GetNumberOfEventsToBeProcessed();

  key = "num_events";
  h5writer_->WriteRunInfo(key,  std::to_string(num_events).c_str());
  key = "saved_events";
  h5writer_->WriteRunInfo(key,  std::to_string(saved_evts_).c_str());

  if (save_ie_numb_) {
    key = "interacting_events";
    h5writer_->WriteRunInfo(key,  std::to_string(interacting_evts_).c_str());
  }

  std::map<G4String, G4double>::const_iterator it;
  for (it = sensdet_bin_.begin(); it != sensdet_bin_.end(); ++it) {
    h5writer_->WriteRunInfo((it->first + "_binning").c_str(),
                           (std::to_string(it->second/microsecond)+" mus").c_str());
  }

  SaveConfigurationInfo(init_macro_);
  for (unsigned long i=0; i<macros_.size(); i++) {
    SaveConfigurationInfo(macros_[i]);
  }
  for (unsigned long i=0; i<delayed_macros_.size(); i++) {
    SaveConfigurationInfo(delayed_macros_[i]);
  }
  for (unsigned long i=0; i<secondary_macros_.size(); i++) {
    SaveConfigurationInfo(secondary_macros_[i]);
  }

  return true;
}

void PersistencyManager::SaveConfigurationInfo(G4String file_name)
{
  std::ifstream history(file_name, std::ifstream::in);
  while (history.good()) {

    G4String line;
    std::getline(history, line);
    if (line.empty())
      continue;
    if (line[0] == '#')
      continue;

    std::stringstream ss(line);
    G4String key, value;
    std::getline(ss, key, ' ');
    std::getline(ss, value);

    if (key != "") {
      auto found_binning = key.find("binning");
      auto found_other_macro = key.find("/control/execute");
      if ((found_binning == std::string::npos) &&
          (found_other_macro == std::string::npos)) {
        if (key[0] == '\n') {
          key.erase(0, 1);
        }
	h5writer_->WriteRunInfo(key.c_str(), value.c_str());
      }

      if (found_other_macro != std::string::npos) {
        auto existing = std::find(secondary_macros_.begin(), secondary_macros_.end(), value);
        if (existing == secondary_macros_.end())
          secondary_macros_.push_back(value);
      }
    }

  }

  history.close();
}
