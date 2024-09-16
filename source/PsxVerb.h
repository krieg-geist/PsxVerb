#pragma once

#include <cstdint>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <algorithm>

// Platform-independent definition of pi
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SPU_REV_PRESET_LONGEST_COUNT (0x18040 / 2)


class PsxVerb {
public:
    PsxVerb();
    ~PsxVerb();

    void init(float sampleRate);

    void process(float* leftBuffer, float* rightBuffer, int numSamples);
    void setPreset(int presetIndex);
    void setWetGain(float newWet);
    void setDryGain(float newDry);
    void setMasterGain(float gain);

private:
    static constexpr int NUM_PRESETS = 10;
    static constexpr float SPU_REV_RATE = 22050.0f;
    

    typedef struct PsxVerbPreset {
        uint16_t dAPF1;
        uint16_t dAPF2;
        int16_t  vIIR;
        int16_t  vCOMB1;
        int16_t  vCOMB2;
        int16_t  vCOMB3;
        int16_t  vCOMB4;
        int16_t  vWALL;
        int16_t  vAPF1;
        int16_t  vAPF2;
        uint16_t mLSAME;
        uint16_t mRSAME;
        uint16_t mLCOMB1;
        uint16_t mRCOMB1;
        uint16_t mLCOMB2;
        uint16_t mRCOMB2;
        uint16_t dLSAME;
        uint16_t dRSAME;
        uint16_t mLDIFF;
        uint16_t mRDIFF;
        uint16_t mLCOMB3;
        uint16_t mRCOMB3;
        uint16_t mLCOMB4;
        uint16_t mRCOMB4;
        uint16_t dLDIFF;
        uint16_t dRDIFF;
        uint16_t mLAPF1;
        uint16_t mRAPF1;
        uint16_t mLAPF2;
        uint16_t mRAPF2;
        int16_t  vLIN;
        int16_t  vRIN;
    } PsxVerbPreset;

    void loadPreset(int presetIndex);

    static float avg (float a, float b)
    {
        return (a + b) / 2.0f;
    }

    static float clampf (float v, float lo, float hi)
    {
        if (v < lo)
            return lo;
        if (v > hi)
            return hi;
        return v;
    }

    static int16_t f2s (float v)
    {
        return (int16_t) (clampf (v * 32768.0f, -32768.0f, 32767.0f));
    }

    static float s2f (int16_t v)
    {
        return (float) (v) / 32768.0f;
    }

    /* convert iir filter constant to center frequency */
    static float alpha2fc (float alpha, float samplerate)
    {
        const double dt = 1.0 / samplerate;
        const double fc_inv = 2.0 * M_PI * (dt / alpha - dt);
        return (float) (1.0 / fc_inv);
    }

    /* convert center frequency to iir filter constant */
    static float fc2alpha (float fc, float samplerate)
    {
        const double dt = 1.0 / samplerate;
        const double rc = 1.0 / (2.0 * M_PI * fc);
        return (float) (dt / (rc + dt));
    }

    static uint32_t ceilpower2 (uint32_t x)
    {
        x--;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x++;
        return x;
    }

    float rate;
    float* spu_buffer;
    uint32_t spu_buffer_count;
    uint32_t spu_buffer_count_mask;
    uint32_t BufferAddress;

    float dry, wet, master;
    PsxVerbPreset preset;
    int preset_index;

    // Reverb parameters
    uint32_t dAPF1, dAPF2;
    float vIIR, vCOMB1, vCOMB2, vCOMB3, vCOMB4, vWALL, vAPF1, vAPF2;
    uint32_t mLSAME, mRSAME, mLCOMB1, mRCOMB1, mLCOMB2, mRCOMB2;
    uint32_t dLSAME, dRSAME, mLDIFF, mRDIFF, mLCOMB3, mRCOMB3, mLCOMB4, mRCOMB4;
    uint32_t dLDIFF, dRDIFF, mLAPF1, mRAPF1, mLAPF2, mRAPF2;
    float vLIN, vRIN;

    static const uint16_t presets[NUM_PRESETS][0x20];
};

