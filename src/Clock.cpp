#include "Clock.h"

namespace Clock {
namespace {

uint32_t offset = 0;

}  // namespace

uint32_t now() {
    return millis() + offset;
}

void addSleep(uint32_t ms) {
    offset += ms;
}

}  // namespace Clock
