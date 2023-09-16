/*
 * Conduit - a series of demonstration and fun plugins
 *
 * Copyright 2023 Paul Walker and authors in github
 *
 * This file you are viewing now is released under the
 * MIT license, but the assembled program which results
 * from compiling it has GPL3 dependencies, so the total
 * program is a GPL3 program. More details to come.
 *
 * Basically before I give this to folks, document this bit and
 * replace these headers
 *
 */

#ifndef CONDUIT_SRC_CONDUIT_SHARED_SPSC_QUEUE_H
#define CONDUIT_SRC_CONDUIT_SHARED_SPSC_QUEUE_H

// Thanks to Alex Droste / 0ax1 for this compact implementation
#include <cstdint>
#include <array>
#include <atomic>

namespace sst::conduit::shared
{
template <typename T, uint16_t S> class alignas(16) SPSCQueue
{
  private:
    std::array<T, S> data_{};
    std::atomic<uint16_t> readIdx_ = 0;
    std::atomic<uint16_t> writeIdx_ = 0;
    const uint16_t wrapMask_ = S - 1;

    // Check whether read-write indices and the underlying index type wrap around in sync.
    static_assert((static_cast<uint64_t>(std::numeric_limits<uint16_t>::max()) + 1) % S == 0);

  public:
    using value_type = T;

    void push(T &&data)
    {
        data_[writeIdx_ & wrapMask_] = data;
        writeIdx_.fetch_add(1);
    }

    void push(const T &data)
    {
        data_[writeIdx_ & wrapMask_] = data;
        writeIdx_.fetch_add(1);
    }

    T pop() { return data_[readIdx_.fetch_add(1) & wrapMask_]; }

    bool isEmpty() const { return readIdx_.load() == writeIdx_.load(); }
};
} // namespace sst::conduit::shared
#endif // CONDUIT_SPSC_QUEUE_H
