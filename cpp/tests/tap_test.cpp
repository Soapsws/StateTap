#include "statetap/tap.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

struct FakeClock {
    static inline std::int64_t now_ns = 0;
    static std::chrono::steady_clock::time_point now() noexcept {
        return std::chrono::steady_clock::time_point(std::chrono::nanoseconds(now_ns));
    }
};

struct FakeSink {
    int registrations = 0;
    std::string topic;
    std::vector<int> values;
    statetap::TapSettings settings;

    template <typename T>
    statetap::SourceHandle register_source(statetap::SourceInfo info) {
        static_assert(std::is_same_v<T, int>);
        ++registrations;
        topic = info.topic;
        assert(info.line > 0);
        return {7, &settings};
    }

    void submit(statetap::SourceId id, const int& value, std::int64_t timestamp_ns) {
        assert(id == 7);
        assert(timestamp_ns == FakeClock::now_ns);
        values.push_back(value);
    }
};

int main() {
    FakeClock::now_ns = 0;
    FakeSink sink;
    statetap::Tap<int, FakeSink, FakeClock> tap("counter", sink);
    assert(!tap.observe(1));
    assert(sink.registrations == 1);
    assert(sink.topic == "counter");
    assert(sink.values.empty());

    sink.settings.set_min_interval(std::chrono::nanoseconds(100));
    sink.settings.set_active(true);
    assert(tap.observe(2));
    FakeClock::now_ns = 50;
    assert(!tap.observe(3));
    FakeClock::now_ns = 100;
    assert(tap.observe(4));
    sink.settings.set_active(false);
    assert(!tap.observe(5));
    assert(sink.registrations == 1);
    assert((sink.values == std::vector<int>{2, 4}));

    FakeClock::now_ns = 120;
    sink.settings.set_active(true);
    assert(tap.observe(6));
    assert((sink.values == std::vector<int>{2, 4, 6}));

    FakeSink wrapper_sink;
    statetap::Watched<int, FakeSink, FakeClock> watched("watched", 9, wrapper_sink);
    wrapper_sink.settings.set_active(true);
    watched.set(10);
    assert(watched.get() == 10);
    assert((wrapper_sink.values == std::vector<int>{10}));
}
