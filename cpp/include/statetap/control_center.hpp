#pragma once

#include "statetap/tap.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>

namespace statetap {

struct SourceDescription {
    SourceId id;
    std::string topic;
    std::string file;
    std::uint_least32_t line;
    std::type_index type;
};

enum class ConfigStatus { applied, unknown_source, invalid_interval };

// Output supplies on_source(const SourceDescription&) and
// on_sample<T>(SourceId, const T&, int64_t). It must outlive the center.
// Callbacks must copy any data they need to retain and must handle concurrent
// on_sample calls. The future Transporter will implement this boundary.
template <typename Output>
class ControlCenter {
public:
    explicit ControlCenter(Output& output) noexcept : output_(&output) {}

    ControlCenter(const ControlCenter&) = delete;
    ControlCenter& operator=(const ControlCenter&) = delete;
    ControlCenter(ControlCenter&&) = delete;
    ControlCenter& operator=(ControlCenter&&) = delete;

    template <typename T>
    SourceHandle register_source(SourceInfo info) {
        if (info.topic.empty()) throw std::invalid_argument("source topic cannot be empty");

        SourceId id;
        Entry* stored;
        {
            std::unique_lock lock(registry_mutex_);
            if (next_id_ == 0) throw std::overflow_error("source IDs exhausted");
            id = next_id_++;
            auto entry = std::make_unique<Entry>(SourceDescription{
                id, std::string(info.topic), std::string(info.file), info.line,
                std::type_index(typeid(T))});
            stored = entry.get();
            sources_.emplace(id, std::move(entry));
        }

        try {
            output_->on_source(stored->description);
        } catch (...) {
            std::unique_lock lock(registry_mutex_);
            sources_.erase(id);
            throw;
        }
        return {id, &stored->settings};
    }

    ConfigStatus set_active(SourceId id, bool active) {
        std::shared_lock lock(registry_mutex_);
        const auto it = sources_.find(id);
        if (it == sources_.end()) return ConfigStatus::unknown_source;
        it->second->settings.set_active(active);
        return ConfigStatus::applied;
    }

    ConfigStatus set_min_interval(SourceId id, std::chrono::nanoseconds interval) {
        if (interval.count() < 0) return ConfigStatus::invalid_interval;
        std::shared_lock lock(registry_mutex_);
        const auto it = sources_.find(id);
        if (it == sources_.end()) return ConfigStatus::unknown_source;
        it->second->settings.set_min_interval(interval);
        return ConfigStatus::applied;
    }

    // False means the ID is unknown or the value type differs from registration.
    template <typename T>
    bool submit(SourceId id, const T& value, std::int64_t timestamp_ns) {
        {
            std::shared_lock lock(registry_mutex_);
            const auto it = sources_.find(id);
            if (it == sources_.end() || it->second->description.type != std::type_index(typeid(T)))
                return false;
        }
        output_->on_sample(id, value, timestamp_ns);
        return true;
    }

    std::optional<SourceDescription> describe(SourceId id) const {
        std::shared_lock lock(registry_mutex_);
        const auto it = sources_.find(id);
        if (it == sources_.end()) return std::nullopt;
        return it->second->description;
    }

    std::size_t source_count() const {
        std::shared_lock lock(registry_mutex_);
        return sources_.size();
    }

private:
    struct Entry {
        explicit Entry(SourceDescription source) : description(std::move(source)) {}
        SourceDescription description;
        TapSettings settings;
    };

    Output* output_;  // Non-owning. Output must outlive this center.
    mutable std::shared_mutex registry_mutex_;
    std::unordered_map<SourceId, std::unique_ptr<Entry>> sources_;
    SourceId next_id_{1};
};

}  // namespace statetap

