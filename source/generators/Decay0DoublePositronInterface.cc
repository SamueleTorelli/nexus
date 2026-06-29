// ----------------------------------------------------------------------------
// nexus | Decay0DoublePositronInterface.cc
//
// This class is a primary generator for a Xe-136 double-beta-like process,
// interfacing the DECAY0 C++ code and converting the generated electrons to
// positrons while preserving the original decay spectrum.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#include "Decay0DoublePositronInterface.h"

#include "DetectorConstruction.h"
#include "GeometryBase.h"
#include "FactoryBase.h"

#include <G4GenericMessenger.hh>
#include <G4RunManager.hh>
#include <G4ParticleTable.hh>
#include <G4ParticleDefinition.hh>
#include <G4PrimaryVertex.hh>
#include <G4Event.hh>
#include "decay0.h"

#include <CLHEP/Units/SystemOfUnits.h>
#include <cmath>
#include <iostream>

using namespace nexus;
using namespace CLHEP;

REGISTER_CLASS(Decay0DoublePositronInterface, G4VPrimaryGenerator)

Decay0DoublePositronInterface::Decay0DoublePositronInterface():
  G4VPrimaryGenerator(), msg_(0), decay_file_("th-e1-spectrum.dat"),
  region_(""), decay0_(0), Xe136DecayMode_(1), Ba136FinalState_(0),
  qValueMeV_(2.45783), energyThreshold_(0.), myEventCounter_(0), geom_(0)
{
  msg_ = new G4GenericMessenger(this, "/Generator/Decay0DoublePositronInterface/",
    "Control commands of the DECAY0 positron interface.");

  msg_->DeclareProperty("region", region_,
                        "Region of the geometry where the vertex will be generated.");
  msg_->DeclareProperty("decay_file", decay_file_,
                        "Name of the file with the decay info");

  msg_->DeclareMethod("EnergyThreshold", &Decay0DoublePositronInterface::SetEnergyThreshold, "");
  msg_->DeclareMethod("Xe136DecayMode", &Decay0DoublePositronInterface::SetXe136DecayMode, "");
  msg_->DeclareMethod("Ba136FinalState", &Decay0DoublePositronInterface::SetBa136FinalState, "");
  msg_->DeclareMethod("QValue", &Decay0DoublePositronInterface::SetQValue, "");

  DetectorConstruction* detConst = (DetectorConstruction*)
    G4RunManager::GetRunManager()->GetUserDetectorConstruction();
  geom_ = detConst->GetGeometry();
}

Decay0DoublePositronInterface::~Decay0DoublePositronInterface()
{
  if (fOutDebug_.is_open()) fOutDebug_.close();
  if (decay0_ != 0) delete decay0_;
  delete msg_;
}

void Decay0DoublePositronInterface::GeneratePrimaryVertex(G4Event* event)
{
  if (decay0_ == 0) {
    const std::string XeName("Xe136");
    decay0_ = new decay0(XeName, Ba136FinalState_, Xe136DecayMode_, decay_file_);
  }

  if (qValueMeV_ > 0.) {
    decay0_->SetQValue(qValueMeV_);
  }

  std::vector<decay0Part> theParts;
  decay0_->decay0DoIt(theParts);

  double eTotKin = 0.;
  for (std::vector<decay0Part>::const_iterator itp = theParts.begin();
       itp != theParts.end(); itp++) {
    if (std::abs(itp->pdgCode_) == 11) eTotKin += itp->energy_;
  }

  const bool keepEvt = eTotKin > energyThreshold_;
  myEventCounter_++;

  G4cout << "[Decay0DoublePositronInterface] event " << event->GetEventID()
         << " positron energy sum = " << eTotKin/MeV << " MeV" << G4endl;

  if (fOutDebug_.is_open() && keepEvt) {
    int k = 0;
    for (std::vector<decay0Part>::const_iterator itp = theParts.begin();
         itp != theParts.end(); itp++, k++) {
      fOutDebug_ << " " << myEventCounter_ << " " << k << " " << itp->pdgCode_ << " "
                 << itp->pmom_[0] << " " << itp->pmom_[1] << " " << itp->pmom_[2] << " "
                 << itp->energy_ << " " << itp->time_ << std::endl;
    }
  }

  if (!keepEvt) return;

  G4ThreeVector particle_position = geom_->GenerateVertex(region_);

  for (std::vector<decay0Part>::const_iterator itp = theParts.begin();
       itp != theParts.end(); itp++) {
    int pdgCode = itp->pdgCode_;
    if (pdgCode == 11) pdgCode = -11;

    G4ParticleDefinition* g4code =
      G4ParticleTable::GetParticleTable()->FindParticle(pdgCode);
    G4PrimaryParticle* particle =
      new G4PrimaryParticle(g4code, MeV*itp->pmom_[0], MeV*itp->pmom_[1], MeV*itp->pmom_[2]);

    particle->SetMass(g4code->GetPDGMass());
    particle->SetCharge(g4code->GetPDGCharge());

    G4PrimaryVertex* vertex =
      new G4PrimaryVertex(particle_position, itp->time_*second);
    vertex->SetPrimary(particle);
    event->AddPrimaryVertex(vertex);
  }
}
