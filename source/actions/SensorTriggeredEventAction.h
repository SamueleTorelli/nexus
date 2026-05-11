#ifndef SENSOR_TRIGGERED_EVENT_ACTION_H
#define SENSOR_TRIGGERED_EVENT_ACTION_H

#include <G4UserEventAction.hh>

class G4Event;

namespace nexus {
  class SensorTriggeredEventAction: public G4UserEventAction
  {
  public:
    SensorTriggeredEventAction();
    ~SensorTriggeredEventAction() override;

    void BeginOfEventAction(const G4Event*) override {}
    void EndOfEventAction(const G4Event* event) override;
  };
} // namespace nexus

#endif

