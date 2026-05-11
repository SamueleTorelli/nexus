// ----------------------------------------------------------------------------
// nexus | PETCsI.cc
//
// This class describes a pair of PETCsI crystals with a radioactive source in the middle
//
// ----------------------------------------------------------------------------

#include "PETCsI.h"
#include "MaterialsList.h"
#include "Visibilities.h"
#include "OpticalMaterialProperties.h"
#include "IonizationSD.h"

#include <G4Box.hh>
#include <G4NistManager.hh>
#include <G4Material.hh>
#include <G4LogicalVolume.hh>
#include <G4PVPlacement.hh>
#include <G4VisAttributes.hh>
#include <G4GenericMessenger.hh>
#include <G4Box.hh>
#include <G4SDManager.hh>

#include "FactoryBase.h"

#include <CLHEP/Units/SystemOfUnits.h>

using namespace nexus;

REGISTER_CLASS(PETCsI, GeometryBase)

namespace nexus
{

  using namespace CLHEP;

  PETCsI::PETCsI() : GeometryBase(),
                                   crystal_material_("CsI"),
                                   crystal_width_(48 * mm),
                                   crystal_length_(2.),
                                   pet_diameter_(60 * cm),
                                   pet_length_(76.8 * cm),
                                   phantom_("sensitivity"),
                                   mixed_(false)
  {
    /// Messenger
    msg_ = new G4GenericMessenger(this, "/Geometry/PETCsI/",
                                  "Control commands of geometry PETCsI.");
    msg_->DeclareProperty("crystal_material", crystal_material_, "Crystal material (CsI, CsITl, LYSO, BGO)");
    G4GenericMessenger::Command&  width_cmd = msg_->DeclareProperty("crystal_width", crystal_width_, "Crystal width");
    width_cmd.SetUnitCategory("Length");
    msg_->DeclareProperty("crystal_length", crystal_length_, "Crystal length (in radiation lengths)");
    G4GenericMessenger::Command& diameter_cmd = msg_->DeclareProperty("pet_diameter", pet_diameter_, "PET inner diameter");
    diameter_cmd.SetUnitCategory("Length");
    G4GenericMessenger::Command& pet_length_cmd = msg_->DeclareProperty("pet_length", pet_length_, "PET length");
    pet_length_cmd.SetUnitCategory("Length");
    msg_->DeclareProperty("phantom", phantom_, "Phantom to be placed inside PET ('sensitivity', 'jaszczak', 'necr', 'none')");
    msg_->DeclareProperty("mixed", mixed_, "Use mixed BGO and CsI (default false)");
  }

  PETCsI::~PETCsI()
  {
    delete msg_;
  }

  void PETCsI::Construct()
  {

    cylindrical_shell_ = new CylinderPointSampler2020(0, pet_diameter_ / 2. - 1 * cm, pet_length_ / 2., 0, 2 * M_PI);

    G4double size = 2 * m;
    G4Box *lab_solid =
        new G4Box("LAB", size / 2., size / 2., size / 2.);

    G4Material *air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");

    G4LogicalVolume *lab_logic = new G4LogicalVolume(lab_solid, air, "LAB");

    lab_logic->SetVisAttributes(G4VisAttributes::GetInvisible());
    new G4PVPlacement(
            0, G4ThreeVector(), lab_logic, "lab", 0, false, 0, true);

    // Set this volume as the wrapper for the whole geometry
    // (i.e., this is the volume that will be placed in the world)
    this->SetLogicalVolume(lab_logic);
    // G4Box *crystal =
    //     new G4Box("CRYSTAL", crystal_width_ / 2., crystal_width_ / 2., crystal_length_ / 2.);

    // G4LogicalVolume *crystal_logic =
    //     new G4LogicalVolume(crystal,
    //                         material,
    //                         "CRYSTAL");
    // crystal_logic->SetVisAttributes(nexus::LightBlueAlpha());
    G4SDManager* sdmgr = G4SDManager::GetSDMpointer();

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

    G4Material *bgo = G4NistManager::Instance()->FindOrBuildMaterial("G4_BGO");
    bgo->SetMaterialPropertiesTable(opticalprops::BGO());

    G4Box *crystal = new G4Box("CRYSTAL", crystal_width_ / 2., crystal_width_ / 2., crystal_length_ / 2);
    G4Box *crystal_bgo = new G4Box("CRYSTAL", crystal_width_ / 2., crystal_width_ / 2., 11.4 * mm);

    // monolithic_csi_ = new PETElement(crystal_material_, crystal_width_,  crystal_length_);
    // monolithic_csi_->Construct();
    // G4double module_width = monolithic_csi_->GetDimensions().x();
    // G4double module_length = monolithic_csi_->GetDimensions().z();
    G4int angles = floor(pet_diameter_ * M_PI / crystal_width_);
    G4double step = 2. * pi / angles;
    G4RotationMatrix *rot = new G4RotationMatrix();
    rot->rotateX(90 * deg);
    rot->rotateZ(180 * deg);

    G4int rings = floor(pet_length_ / crystal_width_);
    G4LogicalVolume *crystal_logic_bgo = new G4LogicalVolume(crystal_bgo,
                        bgo,
                        "BGO");
    crystal_logic_bgo->SetVisAttributes(nexus::RedAlpha());
    G4LogicalVolume *crystal_logic = new G4LogicalVolume(crystal,
                        material,
                        "CSI");
    crystal_logic->SetVisAttributes(nexus::LightBlueAlpha());

    for (G4int iring=0; iring < rings; iring++) {
      for (G4int itheta=0; itheta < angles; itheta++) {
        G4float theta = 2 * M_PI / angles * itheta;
        std::string label = std::to_string(iring*angles + itheta);

        G4double y = (pet_diameter_ / 2. + crystal_length_ / 2) * std::cos(theta);
        G4double x = (pet_diameter_ / 2. + crystal_length_ / 2) * std::sin(theta);
        G4double z = -pet_length_ / 2 + iring * crystal_width_ + crystal_width_ / 2;

        G4cout << "CRYSTAL" << iring*angles + itheta << " " << x << " " << y << " " << z << " " << G4endl;

        IonizationSD *ionisd = nullptr;

        if ((mixed_) && (z < pet_length_/6) && (z>-pet_length_/6)) {
          y = (pet_diameter_ / 2. + 11.4 * mm) * std::cos(theta);
          x = (pet_diameter_ / 2. + 11.4 * mm) * std::sin(theta);

          // ionisd = new IonizationSD("BGO"+label);
          new G4PVPlacement(G4Transform3D(*rot, G4ThreeVector(x, y, z)),
                  crystal_logic_bgo, "BGO"+label, lab_logic,
                  true, iring*angles + itheta, false);
        } else {
          // ionisd = new IonizationSD("CSI"+label);
          new G4PVPlacement(G4Transform3D(*rot, G4ThreeVector(x, y, z)),
                  crystal_logic, "CSI"+label, lab_logic,
                  true, iring*angles + itheta, false);
        }

        // sdmgr->AddNewDetector(ionisd);
        // crystal_logic->SetSensitiveDetector(ionisd);

        rot->rotateZ(-step);
      }
    }

    IonizationSD* ionisd_csi = new IonizationSD("CSI");
    sdmgr->AddNewDetector(ionisd_csi);
    crystal_logic->SetSensitiveDetector(ionisd_csi);

    IonizationSD* ionisd_bgo = new IonizationSD("BGO");
    sdmgr->AddNewDetector(ionisd_bgo);
    crystal_logic_bgo->SetSensitiveDetector(ionisd_bgo);

    // IonizationSD* ionisd_phantom = new IonizationSD("PHANTOM");
    // sdmgr->AddNewDetector(ionisd_phantom);

    if (phantom_ == "sensitivity") {
      nema_sensitivity_ = new NEMASensitivity();
      nema_sensitivity_->Construct();
      G4LogicalVolume *phantom_logic = nema_sensitivity_->GetLogicalVolume();
      new G4PVPlacement(0, G4ThreeVector(0, 0, 0), phantom_logic, "SENSITIVITY",
                        lab_logic, false, 0, true);
      // phantom_logic->SetSensitiveDetector(ionisd_phantom);
    } else if (phantom_ == "jaszczak") {
      jas_phantom_ = new JaszczakPhantom();
      jas_phantom_->Construct();
      G4LogicalVolume* phantom_logic = jas_phantom_->GetLogicalVolume();
      new G4PVPlacement(0, G4ThreeVector(0, 0, 0), phantom_logic, "JASZCZAK",
                        lab_logic, false, 0, true);
      // phantom_logic->SetSensitiveDetector(ionisd_phantom);
    } else if (phantom_ == "necr") {
      nema_necr_ = new NEMANECR();
      nema_necr_->Construct();
      G4LogicalVolume* phantom_logic = nema_necr_->GetLogicalVolume();
      new G4PVPlacement(0, G4ThreeVector(0, 0, 0), phantom_logic, "NECR",
                        lab_logic, false, 0, true);
      // phantom_logic->SetSensitiveDetector(ionisd_phantom);
    }
  }

  G4ThreeVector PETCsI::GenerateVertex(const G4String &region) const
  {

    if (region == "CYLINDRICAL_SHELL") {
      return cylindrical_shell_->GenerateVertex("OUTER_SURFACE");
    } else if (region == "PHANTOM") {
      if (phantom_ == "sensitivity") return nema_sensitivity_->GenerateVertex("VOLUME");
      if (phantom_ == "jaszczak") return jas_phantom_->GenerateVertex("VOLUME");
      if (phantom_ == "necr") return nema_necr_->GenerateVertex("VOLUME");
      else {
        G4ThreeVector vertex(0, 0, 0);
        return vertex;
      }
    } else {
      G4ThreeVector vertex(0, 0, 0);
      return vertex;
    }

  }

}