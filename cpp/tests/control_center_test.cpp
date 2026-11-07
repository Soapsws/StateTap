#include "statetap/control_center.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <vector>

struct RecordingOutput {
    std::vector<statetap::SourceDescription> announced;
    std::vector<int> values;
    std::vector<std::int64_t> timestamps;
    bool reject_next_source = false;
    std::function<void(statetap::SourceId)> after_announce;

    void on_source(const statetap::SourceDescription& source) {
        if (reject_next_source) {
            reject_next_source = false;
            throw std::runtime_error("fake receiver failure");
        }
        announced.push_back(source);
        if (after_announce) after_announce(source.id);
    }

    template <typename T>
    void on_sample(statetap::SourceId, const T& value, std::int64_t timestamp_ns) {
        values.push_back(static_cast<int>(value));
        timestamps.push_back(timestamp_ns);
    }
};

int main() {
    RecordingOutput output;
    statetap::ControlCenter center(output);
    output.after_announce = [&](statetap::SourceId id) {
        assert(center.describe(id));  // Downstream may query the CC during announcement.
    };

    auto first = center.register_source<int>({"robot/speed", "robot.cpp", 12});
    auto second = center.register_source<double>({"robot/speed", "robot.cpp", 27});
    assert(first.id != 0 && first.id != second.id);
    assert(first.settings != nullptr && second.settings != nullptr);
    assert(first.settings != second.settings);
    assert(center.source_count() == 2);
    assert(output.announced.size() == 2);

    const auto description = center.describe(first.id);
    assert(description);
    assert(description->topic == "robot/speed");
    assert(description->file == "robot.cpp");
    assert(description->line == 12);
    assert(description->type == std::type_index(typeid(int)));
    assert(!center.describe(999999));

    assert(center.set_active(first.id, true) == statetap::ConfigStatus::applied);
    assert(first.settings->is_active());
    assert(!second.settings->is_active());
    assert(center.set_active(999999, true) == statetap::ConfigStatus::unknown_source);
    assert(center.set_min_interval(first.id, std::chrono::nanoseconds(-1)) ==
           statetap::ConfigStatus::invalid_interval);
    assert(center.set_min_interval(999999, std::chrono::nanoseconds(10)) ==
           statetap::ConfigStatus::unknown_source);
    assert(center.set_min_interval(first.id, std::chrono::nanoseconds(100)) ==
           statetap::ConfigStatus::applied);

    assert(center.submit(first.id, 42, 1234));
    assert(!center.submit(first.id, 42.0, 1235));  // Registered type differs.
    assert(!center.submit(999999, 42, 1236));
    assert((output.values == std::vector<int>{42}));
    assert((output.timestamps == std::vector<std::int64_t>{1234}));

    // Growing the registry must not invalidate a handle's settings pointer.
    for (int i = 0; i < 256; ++i)
        center.register_source<int>({"extra", "test.cpp", 1});
    assert(first.settings->is_active());
    assert(first.settings->claim_if_due(10));
    assert(!first.settings->claim_if_due(50));
    assert(first.settings->claim_if_due(110));

    output.reject_next_source = true;
    bool failed = false;
    try {
        center.register_source<int>({"rejected", "test.cpp", 2});
    } catch (const std::runtime_error&) {
        failed = true;
    }
    assert(failed);
    assert(center.source_count() == 258);
}

