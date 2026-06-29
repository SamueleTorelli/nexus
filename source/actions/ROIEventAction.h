// ----------------------------------------------------------------------------
// nexus | ROIEventAction.h
//
// Event action that counts every event but stores only events whose primary
// positron-pair energy sum exceeds a configurable threshold.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#ifndef ROI_EVENT_ACTION_H
#define ROI_EVENT_ACTION_H

#include <G4UserEventAction.hh>
#include <globals.hh>

class G4Event;
class G4GenericMessenger;

namespace nexus {

  class ROIEventAction: public G4UserEventAction
  {
  public:
    ROIEventAction();
    ~ROIEventAction();

    void BeginOfEventAction(const G4Event*);
    void EndOfEventAction(const G4Event*);

  private:
    G4GenericMessenger* msg_;
    G4int nevt_, nupdate_;
    G4double positron_energy_threshold_;
  };

} // namespace nexus

#endif
