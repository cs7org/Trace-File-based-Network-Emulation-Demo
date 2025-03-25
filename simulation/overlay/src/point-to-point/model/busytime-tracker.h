#ifndef BUSY_TIME_TRACKER_H
#define BUSY_TIME_TRACKER_H

#include "ns3/simulator.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"

#include <deque>

namespace ns3 {

class BusyTimeTracker : public Object {
public:
    BusyTimeTracker() : busyTime(0), running(false), logId(0) {}

    void StartTransmission();
    void StopTransmission();
    double getBusyRatio(uint64_t trackingWindow);
    void setNode(Ptr<Node> node);

    static ns3::TypeId GetTypeId() {
    static ns3::TypeId tid = ns3::TypeId("BusyTimeTracker");
    return tid;
  }

  virtual ns3::TypeId GetInstanceTypeId() const override {
      return GetTypeId();
  }
private:
    struct BusyPeriod {
        uint64_t timestamp;
        uint64_t durationNs;
    };

    uint64_t busyTime;
    bool running;
    uint64_t lastStartTime;
    uint64_t logId;
    Ptr<Node> m_owner;
    std::deque<BusyPeriod> busyPeriods;

    void cleanUpOldEntries(uint64_t trackingWindow);
};

}

#endif // BUSY_TIME_TRACKER_H
