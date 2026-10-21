#include "LoopProfiler.h"
#include <Arduino.h>

void LoopProfiler::tick(uint32_t nowUs)
{
    if (lastUs_ != kNoValue)
    {
        const uint32_t iterationUs = nowUs - lastUs_;
        totalUs_ += iterationUs;
        minUs_ = min(minUs_, iterationUs);
        maxUs_ = max(maxUs_, iterationUs);
        count_++;

        if (count_ >= reportEvery_)
        {
            Serial.printf("loop: avg %.2f us | min %u us | max %u us (%u iterations)\n",
                          static_cast<double>(totalUs_) / count_,
                          static_cast<unsigned>(minUs_), static_cast<unsigned>(maxUs_),
                          static_cast<unsigned>(count_));
            reset();
        }
    }
    lastUs_ = nowUs;
}

void LoopProfiler::reset()
{
    count_ = 0;
    totalUs_ = 0;
    minUs_ = kNoValue;
    maxUs_ = 0;
}
