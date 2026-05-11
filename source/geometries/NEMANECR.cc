// ----------------------------------------------------------------------------
// petalosim | NEMANECR.cc
//
// This class implements the geometry of a Jaszczak phantom, filled with water.
//
// The PETALO Collaboration
// ----------------------------------------------------------------------------

#include "NEMANECR.h"

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
#include <G4SubtractionSolid.hh>


using namespace nexus;

REGISTER_CLASS(NEMANECR, GeometryBase)

NEMANECR::NEMANECR(): GeometryBase()
{
}


NEMANECR::~NEMANECR()
{
}


void NEMANECR::Construct()
{

  G4Material* water = G4NistManager::Instance()->FindOrBuildMaterial("G4_WATER");
  G4Material* polyethylene = G4NistManager::Instance()->FindOrBuildMaterial("G4_POLYETHYLENE");

  G4Tubs* phantom = new G4Tubs("PHANTOM", 0, 20.3 * cm / 2, 700 * mm / 2, 0, twopi);
  G4LogicalVolume* phantom_logic = new G4LogicalVolume(phantom, polyethylene, "PHANTOM");
  phantom_logic->SetVisAttributes(nexus::WhiteAlpha());

  G4Tubs *capillary = new G4Tubs("CAPILLARY", 0, 3 * mm / 2, 700 * mm / 2, 0, twopi);
  G4LogicalVolume *capillary_logic = new G4LogicalVolume(capillary, water, "CAPILLARY");
  capillary_logic->SetVisAttributes(nexus::LightBlueAlpha());
  G4VPhysicalVolume* capillary_phys = new G4PVPlacement(0, G4ThreeVector(0, 45*mm, 0), capillary_logic, "CAPILLARY", phantom_logic, false, 0);

  // Subtract the capillary from the phantom
  G4SubtractionSolid *phantom_sub_capillary = new G4SubtractionSolid("PHANTOM_SUB_CAPILLARY", phantom, capillary, 0, G4ThreeVector(0, 0, 0));
  G4LogicalVolume *phantom_sub_capillary_logic = new G4LogicalVolume(phantom_sub_capillary, polyethylene, "PHANTOM_SUB_CAPILLARY");
  phantom_sub_capillary_logic->SetVisAttributes(nexus::WhiteAlpha());
  new G4PVPlacement(0, G4ThreeVector(0, 0, 0), phantom_sub_capillary_logic, "PHANTOM_SUB_CAPILLARY", 0, false, 0);
  this->SetLogicalVolume(phantom_logic);
  cyl_gen_ = new CylinderPointSampler2020(capillary_phys);

}


G4ThreeVector NEMANECR::GenerateVertex(const G4String &/*region*/) const
{
  G4ThreeVector vertex(0, 0, 0);
  vertex = cyl_gen_->GenerateVertex("VOLUME");

  return vertex;
}
