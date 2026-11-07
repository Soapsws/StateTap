#include "statetap/control_center.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

struct FakeClock {
    static inline std::int64_t now_ns = 0;
    static std::chrono::steady_clock::time_point now() noexcept {
        return std::chrono::steady_clock::time_point(std::chrono::nanoseconds(now_ns));
    }
};

struct RecordingOutput {
    std::vector<statetap::SourceDescription> sources;
    std::vector<int> values;

    void on_source(const statetap::SourceDescription& source) {
        sources.push_back(source);
    }

    void on_sample(statetap::SourceId, const int& value, std::int64_t) {
        values.push_back(value);
    }
};

int main() {
    RecordingOutput output;
    statetap::ControlCenter center(output);
    statetap::Tap<int, decltype(center), FakeClock> tap("robot/speed", center);

    assert(!tap.observe(1));  // First reach registers, but starts inactive.
    assert(output.sources.size() == 1);
    assert(output.sources[0].topic == "robot/speed");
    const auto id = output.sources[0].id;

    assert(center.set_min_interval(id, std::chrono::nanoseconds(100)) ==
           statetap::ConfigStatus::applied);
    assert(center.set_active(id, true) == statetap::ConfigStatus::applied);
    assert(tap.observe(2));
    FakeClock::now_ns = 50;
    assert(!tap.observe(3));
    FakeClock::now_ns = 100;
    assert(tap.observe(4));
    assert((output.values == std::vector<int>{2, 4}));

    assert(center.set_active(id, false) == statetap::ConfigStatus::applied);
    FakeClock::now_ns = 200;
    assert(!tap.observe(5));
    assert(output.sources.size() == 1);  // Repeated observations do not re-register.

    assert(center.set_active(id, true) == statetap::ConfigStatus::applied);
    assert(tap.observe(6));
    assert((output.values == std::vector<int>{2, 4, 6}));
}
