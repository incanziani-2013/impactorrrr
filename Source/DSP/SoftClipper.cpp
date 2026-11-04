#include "SoftClipper.h"

#include <algorithm>

namespace impactor
{
float SoftClipper::saturate (float x) noexcept
{
    if (x >= 3.0f)  return 1.0f;
    if (x <= -3.0f) return -1.0f;

    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

SoftClipper::Settings SoftClipper::makeSettings (float amount01) noexcept
{
    const float a = std::min (1.0f, std::max (0.0f, amount01));

    Settings s;
    s.drive   = 1.0f + 9.0f * a;                 // 0 dB .. +20 dB aprox
    s.invNorm = 1.0f / saturate (s.drive);
    s.blend   = std::min (1.0f, a * 8.0f);       // entra rapido pero sin salto en 0
    return s;
}

float SoftClipper::process (float x, const Settings& s) noexcept
{
    if (s.blend <= 0.0f)
        return x;

    const float shaped = saturate (x * s.drive) * s.invNorm;
    return x + s.blend * (shaped - x);
}
} // namespace impactor
