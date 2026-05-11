// ----------------------------------------------------------------------------
// nexus | PETElement.cc
//
// This class describes a pair of PETElement crystals with a radioactive source in the middle
//
// ----------------------------------------------------------------------------

#include "PETElement.h"
#include "MaterialsList.h"
#include "Visibilities.h"
#include "OpticalMaterialProperties.h"
#include "SiPM66NoCasing.h"

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

#include "FactoryBase.h"

#include <CLHEP/Units/SystemOfUnits.h>

using namespace nexus;
using namespace CLHEP;

PETElement::PETElement(G4String crystal_material,
                       G4double crystal_width,
                       G4double crystal_length) :
  GeometryBase(),
  crystal_material_(crystal_material),
  crystal_width_(crystal_width),
  crystal_length_(crystal_length)
{
}

PETElement::~PETElement()
{
}



G4ThreeVector PETElement::GetDimensions() const
{
  return dimensions_;
}


void PETElement::Construct()
{
  G4double teflon_thickness = 0.08 * mm;
  G4int teflon_coatings = 5;
  G4double teflon_thickness_tot = teflon_thickness * teflon_coatings;

  SiPM66NoCasing *sipm_geom = new SiPM66NoCasing();
  sipm_geom->Construct();
  G4double sensor_thickness = sipm_geom->GetDimensions().z();
  G4LogicalVolume *sipm_logic = sipm_geom->GetLogicalVolume();

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
    G4Exception("[PETElement]", "Construct()", FatalException,
                "Unknown crystal material!");
  }

  G4Box *pet_element = new G4Box("PET_ELEMENT",
                                 crystal_width_ / 2. + teflon_thickness_tot / 2,
                                 crystal_width_ / 2. + teflon_thickness_tot / 2,
                                 crystal_length_ / 2 + sensor_thickness / 2 + teflon_thickness_tot / 2);

  dimensions_.setX(pet_element->GetXHalfLength() * 2);
  dimensions_.setY(pet_element->GetYHalfLength() * 2);
  dimensions_.setZ(pet_element->GetZHalfLength() * 2);

  G4LogicalVolume *pet_element_logic =
      new G4LogicalVolume(pet_element,
                          G4Material::GetMaterial("G4_AIR"),
                          "PET_ELEMENT");
  pet_element_logic->SetVisAttributes(G4VisAttributes::GetInvisible());

  G4Box *crystal =
      new G4Box("CRYSTAL", crystal_width_ / 2., crystal_width_ / 2., crystal_length_ / 2.);

  G4LogicalVolume *crystal_logic =
      new G4LogicalVolume(crystal,
                          material,
                          "CRYSTAL");
  crystal_logic->SetVisAttributes(nexus::LightGreenAlpha());
  this->SetLogicalVolume(crystal_logic);



  G4VPhysicalVolume *crystal_right = new G4PVPlacement(0, G4ThreeVector(0, 0, -sensor_thickness / 2 + teflon_thickness_tot / 2),
                                                        crystal_logic, "CRYSTAL", pet_element_logic,
                                                        true, 2, true);

  G4Box *teflon_coating_full = new G4Box("TEFLON_FULL", crystal_width_ / 2 + teflon_thickness_tot / 2,
                                                        crystal_width_ / 2 + teflon_thickness_tot / 2,
                                                        crystal_length_ / 2);

  G4SubtractionSolid *teflon_coating = new G4SubtractionSolid("TEFLON", teflon_coating_full, crystal);
  G4LogicalVolume *teflon_logic =
      new G4LogicalVolume(teflon_coating,
                          G4NistManager::Instance()->FindOrBuildMaterial("G4_TEFLON"),
                          "TEFLON_SIDES");

  teflon_logic->SetVisAttributes(nexus::White());

  G4VPhysicalVolume *teflon_full_position = new G4PVPlacement(0, G4ThreeVector(0, 0, -sensor_thickness / 2 + teflon_thickness_tot / 2),
                                                              teflon_logic, "TEFLON_SIDES", pet_element_logic,
                                                              true, 1, true);

  G4OpticalSurface *ptfe_surface = new G4OpticalSurface("PTFE_SURFACE");
  ptfe_surface->SetType(dielectric_LUT);
  ptfe_surface->SetFinish(groundteflonair);
  ptfe_surface->SetModel(LUT);

  new G4LogicalBorderSurface(
      "CRYSTAL_PTFE", crystal_right, teflon_full_position, ptfe_surface);

  G4Box *teflon_back = new G4Box("TEFLON_BACK", crystal_width_ / 2 + teflon_thickness_tot / 2,
                                                crystal_width_ / 2 + teflon_thickness_tot / 2,
                                                teflon_thickness_tot / 2);
  G4LogicalVolume *teflon_back_logic =
      new G4LogicalVolume(teflon_back,
                          G4NistManager::Instance()->FindOrBuildMaterial("G4_TEFLON"),
                          "TEFLON_BACK");
  teflon_back_logic->SetVisAttributes(nexus::White());
  G4VPhysicalVolume *teflon_back_position = new G4PVPlacement(0, G4ThreeVector(0, 0, - crystal_length_ / 2 -  sensor_thickness / 2),
                                                              teflon_back_logic, "TEFLON_BACK", pet_element_logic,
                                                              true, 2, true);
  new G4LogicalBorderSurface(
      "CRYSTAL_PTFE_BACK", crystal_right, teflon_back_position, ptfe_surface);
  G4int n_rows = (int)crystal_width_ / sipm_geom->GetDimensions().x();
  G4int n_cols = (int)crystal_width_ / sipm_geom->GetDimensions().y();

  for (G4int irow = 0; irow < n_rows; irow++)
  {
    for (G4int icol = 0; icol < n_cols; icol++)
    {
      std::string label = std::to_string(irow * n_rows + icol);
      new G4PVPlacement(0, G4ThreeVector(irow * sipm_geom->GetDimensions().x() - crystal_width_ / 2 + sipm_geom->GetDimensions().x() / 2,
                                         icol * sipm_geom->GetDimensions().x() - crystal_width_ / 2 + sipm_geom->GetDimensions().x() / 2,
                                         crystal_length_/2 + teflon_thickness_tot / 2), sipm_logic,
                        "SiPM" + label, pet_element_logic, true, irow * n_rows + icol);
    }
  }
}

