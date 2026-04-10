// ----------------------------------------------------------------------------
// nexus | TrajectoryPoint.cc
//
// This class describes a point (position and time) in the trajectory
// of a particle.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#include "TrajectoryPoint.h"

using namespace nexus;


G4Allocator<TrajectoryPoint> TrjPointAllocator;


TrajectoryPoint::TrajectoryPoint(): 
  position_(0.,0.,0.), time_(0.), energy_(0.)
{
}



TrajectoryPoint::TrajectoryPoint(G4ThreeVector pos, G4double t, G4double e):
  position_(pos), time_(t), energy_(e)
{
}



TrajectoryPoint::TrajectoryPoint(const TrajectoryPoint& other)
{
  *this = other;
}



const TrajectoryPoint& TrajectoryPoint::operator=(const TrajectoryPoint& other)
{
  position_ = other.position_;
  time_     = other.time_;
  energy_   = other.energy_;

  return *this;
}



TrajectoryPoint::~TrajectoryPoint()
{
}



G4double TrajectoryPoint::GetKineticEnergy() const
{
  return energy_;
}



