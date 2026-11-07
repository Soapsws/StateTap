#include "statetap/statetap.hpp"

#include <cstdint>
#include <iostream>

struct ConsoleSink {
    statetap::TapSettings settings;

    template <typename T>
    statetap::SourceHandle register_source(statetap::SourceInfo source) {
        std::cout << "registered " << source.topic << '\n';
        return {1, &settings};
    }

    template <typename T>
    void submit(statetap::SourceId, const T& value, std::int64_t) {
        std::cout << "captured " << value << '\n';
    }
};

int main() {
    ConsoleSink sink;
    statetap::Watched<double, ConsoleSink> speed("robot/speed", 1.5, sink);
    sink.settings.set_active(true);
    speed.set(2.0);
    speed.set(2.5);
    std::cout << "current speed " << speed.get() << '\n';
}

