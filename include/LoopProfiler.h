#pragma once

#include <cstdint>

// Вимірює час однієї ітерації superloop і раз на N ітерацій друкує статистику.
class LoopProfiler
{
public:
    explicit constexpr LoopProfiler(uint32_t reportEvery) : reportEvery_(reportEvery) {}

    // Викликати один раз на початку кожної ітерації loop()
    void tick(uint32_t nowUs);

private:
    static constexpr uint32_t kNoValue = UINT32_MAX;

    void reset();

    const uint32_t reportEvery_;
    uint32_t lastUs_ = kNoValue;
    uint32_t count_ = 0;
    uint64_t totalUs_ = 0;
    uint32_t minUs_ = kNoValue;
    uint32_t maxUs_ = 0;
};
