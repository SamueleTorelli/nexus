// ----------------------------------------------------------------------------
// nexus | SingleDecaySteppingAction.h
//
// This class allows the user to print the total number of photons detected by
// all kinds of photosensors at the end of the run.
// It also shows examples of information that can be accessed at the stepping
// level, so it is useful for debugging.
//
// The  NEXT Collaboration
// ----------------------------------------------------------------------------

#ifndef SINGLE_DECAY_STEPPING_ACTION_H
#define SINGLE_DECAY_STEPPING_ACTION_H

#include <G4UserSteppingAction.hh>
#include <G4GenericMessenger.hh>
#include <globals.hh>
#include <map>

class G4Step;


namespace nexus {

  //  Stepping action to analyze the behaviour of optical photons

  class SingleDecaySteppingAction: public G4UserSteppingAction
  {
  public:
    /// Constructor
    SingleDecaySteppingAction();
    /// Destructor
    ~SingleDecaySteppingAction();

    virtual void UserSteppingAction(const G4Step*);
    void DefineSingle(G4int a, G4int z);

  private:
    G4GenericMessenger* msg_;
    G4int z_;
    G4int a_;
    typedef std::map<G4String, int> detectorCounts;
    detectorCounts my_counts_;
  };

} // namespace nexus

#endif
