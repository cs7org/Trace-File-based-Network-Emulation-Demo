#include "busytime-tracker.h"

#include "ns3/log.h"

#include <iostream>
#include <chrono>
#include <deque>

#define MONITOR_NODE -1

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BusyTimeTracker");

NS_OBJECT_ENSURE_REGISTERED (BusyTimeTracker);

void BusyTimeTracker::StartTransmission() {
    if (m_owner->GetId() == (uint32_t) MONITOR_NODE) {
        NS_LOG_INFO (logId++ <<") ON node " << m_owner-> GetId() << " at " << Simulator::Now().GetNanoSeconds() << " (running before: " << running << ")");
    }

    if (!running) {
        lastStartTime = Simulator::Now().GetNanoSeconds();
        running = true;
    }
}

void BusyTimeTracker::StopTransmission() {
    if (m_owner->GetId() == (uint32_t) MONITOR_NODE) {
        NS_LOG_INFO (logId++ << ") OFF node " << m_owner-> GetId() << " at " << Simulator::Now().GetNanoSeconds() << " (running before: " << running << ", time " << Simulator::Now().GetNanoSeconds() - lastStartTime << ")");
    }

    if (running) {
        uint64_t now = Simulator::Now().GetNanoSeconds();

        if (lastStartTime > now) {
            NS_ABORT_MSG("BusyTimeTracker: Transmission start in future!");
        }

        uint64_t duration = now - lastStartTime;
        busyPeriods.push_back({now, duration});
        busyTime += duration;
        running = false;
    }
}

double BusyTimeTracker::getBusyRatio(uint64_t trackingWindow) {
    cleanUpOldEntries(trackingWindow);

    double real_busy_time = static_cast<double>(busyTime);
    if (running) {
        real_busy_time += Simulator::Now().GetNanoSeconds() - lastStartTime;
    }

    if (real_busy_time > trackingWindow) {
        NS_LOG_WARN ("A link tracking is overloaded:  " << real_busy_time << "ns > " << trackingWindow << "ns (at " << m_owner->GetId() << ")");
    }

    real_busy_time = std::min(real_busy_time, (double) trackingWindow);
    if (real_busy_time == 0) {
        return 0.0;
    }

    return real_busy_time / trackingWindow;
}

void BusyTimeTracker::cleanUpOldEntries(uint64_t trackingWindow) {
    uint64_t now = Simulator::Now().GetNanoSeconds();
    while (!busyPeriods.empty()) {
        uint64_t delay = now - busyPeriods.front().timestamp;
        if (delay < trackingWindow) break;
        busyTime -= busyPeriods.front().durationNs;
        busyPeriods.pop_front();
    }
}

void BusyTimeTracker::setNode(Ptr<Node> node) {
    m_owner = node;
}

}
