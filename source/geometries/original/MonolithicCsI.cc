// ----------------------------------------------------------------------------
// nexus | MonolithicCsI.cc
//
// This class describes a pair of MonolithicCsI crystals with a radioactive source in the middle
//
// ----------------------------------------------------------------------------

#include "MonolithicCsI.h"
#include "MaterialsList.h"
#include "Visibilities.h"
#include "OpticalMaterialProperties.h"
#include "SiPM66NoCasing.h"
#include "SiPM33NoCasing.h"
#include "SiPM44NoCasing.h"
#include "IonizationSD.h"
#include <G4SDManager.hh>

#include <G4Tubs.hh>
#include <G4SubtractionSolid.hh>
#include <G4OpticalSurface.hh>
#include <G4LogicalSkinSurface.hh>
#include <G4LogicalBorderSurface.hh>

#include <G4Box.hh>
#include <G4NistManager.hh>
#include <G4Material.hh>
#include <G4LogicalVolume.hh>
#include <G4PVPlacement.hh>
#include <G4VisAttributes.hh>
#include <G4GenericMessenger.hh>

#include "FactoryBase.h"

#include <CLHEP/Units/SystemOfUnits.h>

using namespace nexus;

REGISTER_CLASS(MonolithicCsI, GeometryBase)

namespace nexus
{

  using namespace CLHEP;

  MonolithicCsI::MonolithicCsI() : GeometryBase(),
                                   crystal_material_("CsI"),
                                   crystal_width_(48 * mm),
                                   crystal_length_(2.),
                                   pixelated_(false),
                                   pixel_size_(6.1 * mm),
                                   back_wrapping_(true)
  {
    /// Messenger
    msg_ = new G4GenericMessenger(this, "/Geometry/MonolithicCsI/",
                                  "Control commands of geometry MonolithicCsI.");
    msg_->DeclareProperty("crystal_material", crystal_material_, "Crystal material (CsI, CsITl, LYSO, BGO)");
    G4GenericMessenger::Command& crystal_width_cmd = msg_->DeclareProperty("crystal_width", crystal_width_, "Crystal width");
    crystal_width_cmd.SetUnitCategory("Length");
    msg_->DeclareProperty("crystal_length", crystal_length_, "Crystal length (in radiation lengths)");
    msg_->DeclareProperty("pixelated", pixelated_, "Pixelated crystal?");
    G4GenericMessenger::Command& pixel_size_cmd = msg_->DeclareProperty("pixel_size", pixel_size_, "Pixel size");
    pixel_size_cmd.SetUnitCategory("Length");
    msg_->DeclareProperty("back_wrapping", back_wrapping_, "Crystal wrapped also on the back side? (true/false)");
  }

  MonolithicCsI::~MonolithicCsI()
  {
    delete msg_;
  }

  void MonolithicCsI::Construct()
  {

    G4double size = 0.5 * m;
    G4Box *lab_solid =
        new G4Box("LAB", size / 2., size / 2., size / 2.);

    G4Material *air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");

    G4LogicalVolume *lab_logic = new G4LogicalVolume(lab_solid, air, "LAB");

    lab_logic->SetVisAttributes(G4VisAttributes::GetInvisible());
    new G4PVPlacement(0, G4ThreeVector(), lab_logic, "lab", 0, false, 0, true);

    // Set this volume as the wrapper for the whole geometry
    // (i.e., this is the volume that will be placed in the world)
    this->SetLogicalVolume(lab_logic);

    G4Material *material = nullptr;

    if (crystal_material_ == "CsI") {
      crystal_length_ *= 18.6 * mm;
      material = G4NistManager::Instance()->FindOrBuildMaterial("G4_CESIUM_IODIDE");
      material->SetMaterialPropertiesTable(opticalprops::CsI());
    } else if (crystal_material_ == "BGO") {
      crystal_length_ *= 11.4 * mm;
      material = G4NistManager::Instance()->FindOrBuildMaterial("G4_BGO");
      material->SetMaterialPropertiesTable(opticalprops::BGO());
    } else if (crystal_material_ == "LYSO") {
      crystal_length_ *= 11.4 * mm;
      material = materials::LYSO();
      material->SetMaterialPropertiesTable(opticalprops::LYSO());
    } else if (crystal_material_ == "CsITl") {
      crystal_length_ *= 18.6 * mm;
      material = G4NistManager::Instance()->FindOrBuildMaterial("G4_CESIUM_IODIDE");
      material->SetMaterialPropertiesTable(opticalprops::CsITl());
    } else {
      G4Exception("[MonolithicCsI]", "Construct()", FatalException,
                  "Unknown crystal material!");
    }

    plane_source_ = new BoxPointSampler(crystal_width_, crystal_width_, 1 * mm, 0, G4ThreeVector(0, 0,   -crystal_length_/2));
    box_source_ = new BoxPointSampler(crystal_width_, crystal_width_, crystal_length_, 0, G4ThreeVector(0, 0, + crystal_length_ / 2));


    G4Box *crystal = nullptr;
    if (!pixelated_) {
      crystal = new G4Box("CRYSTAL", crystal_width_ / 2., crystal_width_ / 2., crystal_length_ / 2.);
    } else {
      crystal = new G4Box("CRYSTAL", pixel_size_ / 2., pixel_size_ / 2., crystal_length_ / 2.);
    }

    G4LogicalVolume *crystal_logic =
        new G4LogicalVolume(crystal,
                            material,
                            "CRYSTAL");
    crystal_logic->SetVisAttributes(nexus::LightBlueAlpha());


    G4VPhysicalVolume *crystal_right = nullptr;
    if (!pixelated_) {
      crystal_right = new G4PVPlacement(0, G4ThreeVector(0, 0,  + crystal_length_ / 2),
                                                          crystal_logic, "CRYSTAL", lab_logic,
                                                          true, 2, true);
    }

    G4double teflon_thickness = 0.08 * mm;
    G4int teflon_coatings = 5;
    G4double teflon_thickness_tot = teflon_thickness * teflon_coatings;
    G4Box *teflon_coating_full = new G4Box("TEFLON_FULL", crystal_width_ / 2 + teflon_thickness_tot / 2, crystal_width_ / 2 + teflon_thickness_tot / 2, crystal_length_ / 2.);

    G4SubtractionSolid *teflon_coating = new G4SubtractionSolid("TEFLON", teflon_coating_full, crystal);
    G4LogicalVolume *teflon_logic =
        new G4LogicalVolume(teflon_coating,
                            G4NistManager::Instance()->FindOrBuildMaterial("G4_TEFLON"),
                            "TEFLON_SIDES");

    teflon_logic->SetVisAttributes(nexus::WhiteAlpha());
    G4OpticalSurface *ptfe_surface = new G4OpticalSurface("PTFE_SURFACE");
    ptfe_surface->SetType(dielectric_LUT);
    ptfe_surface->SetFinish(groundteflonair);
    // ptfe_surface->SetFinish(RoughTeflon_LUT);
    ptfe_surface->SetModel(LUT);

    if (!pixelated_) {

      G4VPhysicalVolume *teflon_full_position = new G4PVPlacement(0, G4ThreeVector(0, 0, crystal_length_ / 2),
                                                                  teflon_logic, "TEFLON_SIDES", lab_logic,
                                                                  true, 1, true);


    // ptfe_surface->SetType(dielectric_dielectric);
    // ptfe_surface->SetFinish(polishedvm2000air);
    // ptfe_surface->SetFinish(polishedfrontpainted);
    // ptfe_surface->SetModel(unified);
    // ptfe_surface->SetMaterialPropertiesTable(opticalprops::PTFE());
//
    // G4OpticalSurface *air_surface = new G4OpticalSurface("CRYSTAL_SURFACE");
    // ptfe_surface->SetType(dielectric_LUTDAVIS);
    // ptfe_surface->SetFinish(Polished_LUT);
    // ptfe_surface->SetModel(DAVIS);

      new G4LogicalBorderSurface(
          "CRYSTAL_PTFE", crystal_right, teflon_full_position, ptfe_surface);
    }
    if (back_wrapping_) {
      G4Box *teflon_back = new G4Box("TEFLON_BACK", crystal_width_ / 2 + teflon_thickness_tot / 2, crystal_width_ / 2 + teflon_thickness_tot / 2, teflon_thickness_tot / 2);
      G4LogicalVolume *teflon_back_logic =
          new G4LogicalVolume(teflon_back,
                              G4NistManager::Instance()->FindOrBuildMaterial("G4_TEFLON"),
                              "TEFLON_BACK");
      teflon_back_logic->SetVisAttributes(nexus::WhiteAlpha());
      if (!pixelated_) {
        G4VPhysicalVolume *teflon_back_position = new G4PVPlacement(0, G4ThreeVector(0, 0, - teflon_thickness_tot / 2),
                                                                    teflon_back_logic, "TEFLON_BACK", lab_logic,
                                                                    true, 2, true);

        new G4LogicalBorderSurface(
            "CRYSTAL_PTFE_BACK", crystal_right, teflon_back_position, ptfe_surface);
      }
    }


    G4SDManager* sdmgr = G4SDManager::GetSDMpointer();


    SiPM66NoCasing *sipm_geom = new SiPM66NoCasing();

    sipm_geom->Construct();
    G4LogicalVolume *sipm_logic = sipm_geom->GetLogicalVolume();
    G4int n_rows = (int)(crystal_width_ / pixel_size_);
    G4int n_cols = (int)(crystal_width_ / pixel_size_);
    for (G4int irow = 0; irow < n_rows; irow++)
    {
      for (G4int icol = 0; icol < n_cols; icol++)
      {
        std::string label = std::to_string(irow * n_cols + icol);
        new G4PVPlacement(0, G4ThreeVector(irow * pixel_size_ - crystal_width_ / 2 + pixel_size_ / 2,
                                           icol * pixel_size_ - crystal_width_ / 2 + pixel_size_ / 2,
                                           crystal_length_ + sipm_geom->GetDimensions().z() / 2),
                          sipm_logic, "SiPM" + label, lab_logic, true, irow * n_cols + icol);
        G4LogicalVolume *crystal_logic =
        new G4LogicalVolume(crystal,
                            material,
                            "CRYSTAL");
        crystal_logic->SetVisAttributes(nexus::LightBlueAlpha());

        if (pixelated_) {
        new G4PVPlacement(0, G4ThreeVector(irow * pixel_size_ - crystal_width_ / 2 + pixel_size_ / 2,
                                           icol * pixel_size_ - crystal_width_ / 2 + pixel_size_ / 2,
                                           crystal_length_ / 2),
                          crystal_logic, "CRYSTAL" + label, lab_logic, true, irow * n_cols + icol);
        IonizationSD* ionisd = new IonizationSD("CRYSTAL" + label);
        sdmgr->AddNewDetector(ionisd);
        crystal_logic->SetSensitiveDetector(ionisd);
        }

      }
    }
  }

  G4ThreeVector MonolithicCsI::GenerateVertex(const G4String &region) const
  {
    G4ThreeVector vertex(0, 0, 0);
    // return vertex;
    if (region == "INSIDE")
      return box_source_->GenerateVertex("INSIDE");
    else if (region == "OUTSIDE")
      return plane_source_->GenerateVertex("INSIDE");
    else
      return vertex;
  }

}