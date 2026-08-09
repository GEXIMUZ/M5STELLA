// M5STELLA compatibility weather shim.
// The visual baseline intentionally matches M5PORKCHOP v0.1.6,
// which had no cloud/rain/wind/night-sky rendering layer.

#include "weather.h"

namespace Weather {

void init() {}
void setMoodLevel(int) {}
void setRaining(bool) {}
void triggerThunderStorm() {}
void update() {}
void draw(M5Canvas&, uint16_t, uint16_t) {}
void drawClouds(M5Canvas&, uint16_t) {}
bool isThunderFlashing() { return false; }
bool isRaining() { return false; }

}  // namespace Weather
