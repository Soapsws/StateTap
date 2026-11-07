#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace statetap {

using SourceId = std::uint64_t;

struct SourceInfo {
    std::string_view topic;
    std::string_view file;
    std::uint_least32_t line;
};

// The receiver owns this stable slot; a tap reads it through its handle.
class TapSettings {
public:
    void set_active(bool active) noexcept {
        active_.store(active, std::memory_order_relaxed);
        if (active) next_due_ns_.store(0, std::memory_order_relaxed);
    }

    bool is_active() const noexcept {
        return active_.load(std::memory_order_relaxed);
    }

    void set_min_interval(std::chrono::nanoseconds interval) {
        if (interval.count() < 0) throw std::invalid_argument("tap interval cannot be negative");
        interval_ns_.store(interval.count(), std::memory_order_relaxed);
        next_due_ns_.store(0, std::memory_order_relaxed);
    }

    bool claim_if_due(std::int64_t now_ns) noexcept {
        if (!is_active()) return false;
        const auto interval_ns = interval_ns_.load(std::memory_order_relaxed);
        auto next_due_ns = next_due_ns_.load(std::memory_order_relaxed);
        for (;;) {
            if (now_ns < next_due_ns) return false;
            if (next_due_ns_.compare_exchange_weak(next_due_ns, now_ns + interval_ns,
                                                   std::memory_order_relaxed)) return true;
        }
    }

private:
    std::atomic<bool> active_{false};
    std::atomic<std::int64_t> interval_ns_{0};
    std::atomic<std::int64_t> next_due_ns_{0};
};

struct SourceHandle {
    SourceId id;
    TapSettings* settings;
};

// Sink supplies register_source<T>(SourceInfo) -> SourceHandle, and
// submit(SourceId, const T&, int64_t). It and the returned settings slot must
// outlive the tap. submit must consume or copy the value before returning.
template <typename T, typename Sink, typename Clock = std::chrono::steady_clock>
class Tap {
public:
    explicit Tap(std::string topic, Sink& sink,
                 std::source_location location = std::source_location::current())
        : topic_(std::move(topic)), sink_(&sink), location_(location) {
        if (topic_.empty()) throw std::invalid_argument("tap topic cannot be empty");
    }

    Tap(const Tap&) = delete;
    Tap& operator=(const Tap&) = delete;
    Tap(Tap&&) = delete;
    Tap& operator=(Tap&&) = delete;

    SourceHandle source_handle() {
        if (registered_.load(std::memory_order_acquire)) return handle_;
        std::lock_guard lock(registration_mutex_);
        if (!registered_.load(std::memory_order_relaxed)) {
            handle_ = sink_->template register_source<T>(
                SourceInfo{topic_, location_.file_name(), location_.line()});
            if (handle_.id == 0 || handle_.settings == nullptr)
                throw std::runtime_error("sink returned invalid source handle");
            registered_.store(true, std::memory_order_release);
        }
        return handle_;
    }

    // Called by the observed program. This never polls or re-evaluates the value.
    bool observe(const T& value) {
        const SourceHandle handle = source_handle();  // First reach reveals the tap.
        if (!handle.settings->is_active()) return false;

        const auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            Clock::now().time_since_epoch()).count();
        if (!handle.settings->claim_if_due(now_ns)) return false;
        sink_->submit(handle.id, value, now_ns);
        return true;
    }

private:
    std::string topic_;
    Sink* sink_;  // Non-owning; the future Control Center will provide this sink.
    std::source_location location_;
    std::mutex registration_mutex_;
    SourceHandle handle_{0, nullptr};
    std::atomic<bool> registered_{false};
};

// A minimal owning wrapper. set() is observable; get() is intentionally read-only.
// More transparent operators must be designed and tested separately.
template <typename T, typename Sink, typename Clock = std::chrono::steady_clock>
class Watched {
public:
    Watched(std::string topic, T initial, Sink& sink,
            std::source_location location = std::source_location::current())
        : value_(std::move(initial)), tap_(std::move(topic), sink, location) {}

    const T& get() const noexcept { return value_; }
    Tap<T, Sink, Clock>& tap() noexcept { return tap_; }

    void set(T value) {
        value_ = std::move(value);
        tap_.observe(value_);
    }

    bool observe() { return tap_.observe(value_); }

private:
    T value_;
    Tap<T, Sink, Clock> tap_;
};

}  // namespace statetap

