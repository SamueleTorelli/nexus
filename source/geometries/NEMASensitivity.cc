// ----------------------------------------------------------------------------
// petalosim | NEMASensitivity.cc
//
// This class implements the geometry of a Jaszczak phantom, filled with water.
//
// The PETALO Collaboration
// ----------------------------------------------------------------------------

#include "NEMASensitivity.h"

#include "FactoryBase.h"
#include "Visibilities.h"
#include "CylinderPointSampler2020.h"

#include <G4Tubs.hh>
#include <G4LogicalVolume.hh>
#include <G4Material.hh>
#include <G4NistManager.hh>
#include <G4PVPlacement.hh>
#include <G4RotationMatrix.hh>
#include <G4VisAttributes.hh>
#include <G4GenericMessenger.hh>
#include <G4TransportationManager.hh>

using namespace nexus;

REGISTER_CLASS(NEMASensitivity, GeometryBase)

NEMASensitivity::NEMASensitivity(): GeometryBase()
{
}


NEMASensitivity::~NEMASensitivity()
{
}


void NEMASensitivity::Construct()
{

  G4double inner_diameters[] = {3.9, 7.0, 10.2, 13.4, 16.6};
  G4double outer_diameters[] = {6.4, 9.5, 12.7, 15.9, 19.1};

  G4Material* air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");
  G4Material* aluminum = G4NistManager::Instance()->FindOrBuildMaterial("G4_Al");
  G4Material* polyethylene = G4NistManager::Instance()->FindOrBuildMaterial("G4_POLYETHYLENE");

  G4Tubs* phantom = new G4Tubs("PHANTOM", 0, outer_diameters[4] * mm, 700 * mm / 2, 0, twopi);
  G4LogicalVolume* phantom_logic = new G4LogicalVolume(phantom, air, "PHANTOM");
  phantom_logic->SetVisAttributes(G4VisAttributes::GetInvisible());
  this->SetLogicalVolume(phantom_logic);
  new G4PVPlacement(0, G4ThreeVector(0, 0, 0), phantom_logic, "PHANTOM", 0, false, 0);

  for (int i = 0; i < 5; i++) {
    std::string label = std::to_string(i);
    G4Tubs* rod = new G4Tubs("ROD" + label, inner_diameters[i] * mm, outer_diameters[i] * mm, 700 * mm / 2, 0, twopi);
    G4LogicalVolume* rod_logic = new G4LogicalVolume(rod, aluminum, "SPHERE");
    rod_logic->SetVisAttributes(nexus::LightGreyAlpha());
    // new G4PVPlacement(0, G4ThreeVector(0, 0, 0), rod_logic, "ROD" + label, phantom_logic, false, i);
  }

  G4Tubs *sleeve = new G4Tubs("SLEEVE", 0, 3. * mm, 700 * mm / 2, 0, twopi);
  G4LogicalVolume *sleeve_logic = new G4LogicalVolume(sleeve, polyethylene, "SLEEVE");
  sleeve_logic->SetVisAttributes(nexus::LightGreenAlpha());
  G4VPhysicalVolume* sleeve_phys = new G4PVPlacement(0, G4ThreeVector(0, 0, 0), sleeve_logic, "SLEEVE", phantom_logic, false, 0);
  cyl_gen_ = new CylinderPointSampler2020(sleeve_phys);


}


G4ThreeVector NEMASensitivity::GenerateVertex(const G4String &/*region*/) const
{
  G4ThreeVector vertex(0, 0, 0);
  vertex = cyl_gen_->GenerateVertex("VOLUME");

  return vertex;
}
