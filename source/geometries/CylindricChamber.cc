// ----------------------------------------------------------------------------
// nexus | CylindricChamber.cc
//
// General-purpose cylindric chamber.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#include "CylindricChamber.h"

#include "PmtR11410.h"
#include "NextNewKDB.h"
#include "MaterialsList.h"
#include "OpticalMaterialProperties.h"
#include "UniformElectricDriftField.h"
#include "IonizationSD.h"
#include "FactoryBase.h"
#include "CylinderPointSampler.h"

#include <G4GenericMessenger.hh>
#include <G4Tubs.hh>
#include <G4LogicalVolume.hh>
#include <G4PVPlacement.hh>
#include <G4SDManager.hh>
#include <G4UserLimits.hh>

#include <CLHEP/Units/SystemOfUnits.h>
#include <CLHEP/Units/PhysicalConstants.h>


namespace nexus {

  REGISTER_CLASS(CylindricChamber, GeometryBase)

  using namespace CLHEP;

  CylindricChamber::CylindricChamber():
    GeometryBase(),
    sc_yield_(25510. * 1/MeV),
    e_lifetime_(1000. * ms),
    pressure_   (13.5 * bar),
    temperature_(293. * kelvin),
    chamber_diam_   (100. * cm),
    chamber_length_ (100. * cm),
    chamber_thickn_ (  1. * cm),
    step_max_(1.0 * mm)
  {
    msg_ = new G4GenericMessenger(this, "/Geometry/CylindricChamber/",
      "Control commands of geometry CylindricChamber.");

    msg_->DeclareProperty("targetmaterial", target_, "Gas being used");

    G4GenericMessenger::Command& pressure_cmd =
      msg_->DeclareProperty("pressure", pressure_, "Xenon pressure");
    pressure_cmd.SetUnitCategory("Pressure");
    pressure_cmd.SetParameterName("pressure", false);
    pressure_cmd.SetRange("pressure>0.");

    G4GenericMessenger::Command& step_cmd =
      msg_->DeclareProperty("step_max", step_max_, "Maximum step length inside ACTIVE region");
    step_cmd.SetUnitCategory("Length");
    step_cmd.SetParameterName("step_max", false);
    step_cmd.SetRange("step_max>0.");
  }



  CylindricChamber::~CylindricChamber()
  {
    delete msg_;
  }



  void CylindricChamber::Construct()
  {
    // CHAMBER ///////////////////////////////////////////////////////

    G4Tubs* chamber_solid =
      new G4Tubs("CHAMBER", 0., (chamber_diam_/2. + chamber_thickn_),
        (chamber_length_/2. + chamber_thickn_), 0., twopi);

    G4LogicalVolume* chamber_logic =
      new G4LogicalVolume(chamber_solid, materials::Steel(), "CHAMBER");

    this->SetLogicalVolume(chamber_logic);


    // TARGET ///////////////////////////////////////////////////////////

    G4Tubs* target_solid =
      new G4Tubs("GAS", 0., chamber_diam_/2., chamber_length_/2., 0., twopi);

    G4Material* a_target;
    if(target_ == "LXe"){
      a_target = materials::LXe();
      a_target->SetMaterialPropertiesTable(opticalprops::LXe());
    } else if(target_ == "GXe"){
      a_target = materials::GXe(pressure_, temperature_);
      a_target->SetMaterialPropertiesTable(
        opticalprops::GXe(pressure_,
        temperature_,
        sc_yield_,
        e_lifetime_));
    }

    G4LogicalVolume* target_logic = new G4LogicalVolume(target_solid, a_target, "GAS");
    
    new G4PVPlacement(0, G4ThreeVector(0.,0.,0.), target_logic, "GAS",
		      chamber_logic, false, 0, true);

        
    // ACTIVE ////////////////////////////////////////////////////////

    const G4double active_diam   = chamber_diam_;
    const G4double active_length = chamber_length_;

    G4Tubs* active_solid =
      new G4Tubs("ACTIVE", 0., active_diam/2., active_length/2., 0, twopi);

    G4LogicalVolume* active_logic =
      new G4LogicalVolume(active_solid, a_target, "ACTIVE");
    active_logic->SetUserLimits(new G4UserLimits(step_max_));

    new G4PVPlacement(0, G4ThreeVector(0.,0.,0.), active_logic, "ACTIVE",
		      target_logic, false, 0, true);

    // Define this volume as an ionization sensitive detector
    IonizationSD* sensdet = new IonizationSD("/CYLINDRIC_CHAMBER/ACTIVE");
    active_logic->SetSensitiveDetector(sensdet);
    G4SDManager::GetSDMpointer()->AddNewDetector(sensdet);      
  }

  G4ThreeVector CylindricChamber::GenerateVertex(const G4String& region) const
  {
    // Keep geometry parameters consistent with Construct()
    if (region == "ACTIVE") {
      CylinderPointSampler sampler(0., chamber_diam_/2., chamber_length_/2., 0., twopi);
      return sampler.GenerateVertex(VOLUME);
    }
    else if (region == "CHAMBER") {
      // Steel shell (barrel + endcaps): sample body vs endcaps by volume
      const G4double inner_radius = chamber_diam_/2.;
      const G4double outer_radius = inner_radius + chamber_thickn_;
      const G4double inner_half_len = chamber_length_/2.;
      const G4double cap_half_len   = chamber_thickn_/2.;

      const G4double body_vol   = CLHEP::pi * (outer_radius*outer_radius - inner_radius*inner_radius) * (2. * inner_half_len);
      const G4double endcaps_vol = 2. * (CLHEP::pi * outer_radius*outer_radius * (2. * cap_half_len));
      const G4double perc_body   = body_vol / (body_vol + endcaps_vol);

      if (G4UniformRand() < perc_body) {
        // Barrel steel
        CylinderPointSampler body_sampler(inner_radius, outer_radius, inner_half_len, 0., twopi);
        return body_sampler.GenerateVertex(VOLUME);
      } else {
        // Endcap steel (choose +Z or -Z)
        const G4double z0 = inner_half_len + cap_half_len;
        const G4double sign = (G4UniformRand() < 0.5) ? -1. : 1.;
        CylinderPointSampler cap_sampler(0., outer_radius, cap_half_len, 0., twopi, nullptr,
                                         G4ThreeVector(0., 0., sign * z0));
        return cap_sampler.GenerateVertex(VOLUME);
      }
    }
    else if (region == "CENTER") {
      // Generate particle at the chamber center
      return G4ThreeVector(0., 0., 0.);
    }
    else {
      G4Exception("[CylindricChamber]", "GenerateVertex()", FatalException,
		  "Unknown vertex generation region!");
      return G4ThreeVector(0.,0.,0.);
    }
  }


} // end namespace nexus
