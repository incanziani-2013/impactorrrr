#pragma once

namespace impactor
{
/**
 * Soft clipper sin estado (no guarda memoria entre muestras).
 *
 * Usa una curva racional tipo tanh (barata en CPU) que llega exactamente a +-1
 * en |x| >= 3, por eso la salida siempre queda acotada.
 *
 * amount: 0..1.  0 = sin efecto (bypass exacto), 1 = saturacion fuerte.
 */
class SoftClipper
{
public:
    struct Settings
    {
        float drive   = 1.0f;   // ganancia previa a la curva
        float invNorm = 1.0f;   // normalizacion para que un pico de 1.0 siga en ~1.0
        float blend   = 0.0f;   // 0 = senal limpia, 1 = totalmente saturada
    };

    /** Calcula los coeficientes. Llamar de vez en cuando, NO por cada muestra. */
    static Settings makeSettings (float amount01) noexcept;

    /** Curva de saturacion pura, acotada a [-1, 1]. */
    static float saturate (float x) noexcept;

    /** Procesa una muestra. */
    static float process (float x, const Settings& s) noexcept;
};
} // namespace impactor
