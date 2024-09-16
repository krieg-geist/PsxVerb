#include "PsxVerb.h"
#include <cstring>
#include <cmath>

PsxVerb::PsxVerb() {
    dry = 1.0f;
    wet = 1.0f;
    master = 1.0f;
    preset_index = 0;
}

PsxVerb::~PsxVerb() {
    delete[] spu_buffer;
}

void PsxVerb::init (float sampleRate)
{
    rate = sampleRate;
    spu_buffer_count = ceilpower2 ((uint32_t) ceil (SPU_REV_PRESET_LONGEST_COUNT * (rate / SPU_REV_RATE)));
    spu_buffer_count_mask = spu_buffer_count - 1;
    spu_buffer = new float[spu_buffer_count];

    BufferAddress = 0;

    memset (spu_buffer, 0, spu_buffer_count * sizeof (float));
    loadPreset (preset_index);
}


void PsxVerb::process(float* leftBuffer, float* rightBuffer, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        const float Lin = vLIN * leftBuffer[i];
        const float Rin = vRIN * rightBuffer[i];

        // Same side reflection
        spu_buffer[(mLSAME + BufferAddress) & spu_buffer_count_mask] =
            (Lin + spu_buffer[(dLSAME + BufferAddress) & spu_buffer_count_mask] * vWALL -
                spu_buffer[(mLSAME + BufferAddress - 1) & spu_buffer_count_mask]) * vIIR +
            spu_buffer[(mLSAME + BufferAddress - 1) & spu_buffer_count_mask];

        spu_buffer[(mRSAME + BufferAddress) & spu_buffer_count_mask] =
            (Rin + spu_buffer[(dRSAME + BufferAddress) & spu_buffer_count_mask] * vWALL -
                spu_buffer[(mRSAME + BufferAddress - 1) & spu_buffer_count_mask]) * vIIR +
            spu_buffer[(mRSAME + BufferAddress - 1) & spu_buffer_count_mask];

        // Different side reflection
        spu_buffer[(mLDIFF + BufferAddress) & spu_buffer_count_mask] =
            (Lin + spu_buffer[(dRDIFF + BufferAddress) & spu_buffer_count_mask] * vWALL -
                spu_buffer[(mLDIFF + BufferAddress - 1) & spu_buffer_count_mask]) * vIIR +
            spu_buffer[(mLDIFF + BufferAddress - 1) & spu_buffer_count_mask];

        spu_buffer[(mRDIFF + BufferAddress) & spu_buffer_count_mask] =
            (Rin + spu_buffer[(dLDIFF + BufferAddress) & spu_buffer_count_mask] * vWALL -
                spu_buffer[(mRDIFF + BufferAddress - 1) & spu_buffer_count_mask]) * vIIR +
            spu_buffer[(mRDIFF + BufferAddress - 1) & spu_buffer_count_mask];

        // Early echo
        float Lout = vCOMB1 * spu_buffer[(mLCOMB1 + BufferAddress) & spu_buffer_count_mask] +
            vCOMB2 * spu_buffer[(mLCOMB2 + BufferAddress) & spu_buffer_count_mask] +
            vCOMB3 * spu_buffer[(mLCOMB3 + BufferAddress) & spu_buffer_count_mask] +
            vCOMB4 * spu_buffer[(mLCOMB4 + BufferAddress) & spu_buffer_count_mask];

        float Rout = vCOMB1 * spu_buffer[(mRCOMB1 + BufferAddress) & spu_buffer_count_mask] +
            vCOMB2 * spu_buffer[(mRCOMB2 + BufferAddress) & spu_buffer_count_mask] +
            vCOMB3 * spu_buffer[(mRCOMB3 + BufferAddress) & spu_buffer_count_mask] +
            vCOMB4 * spu_buffer[(mRCOMB4 + BufferAddress) & spu_buffer_count_mask];

        // Late reverb APF1
        Lout -= vAPF1 * spu_buffer[(mLAPF1 + BufferAddress - dAPF1) & spu_buffer_count_mask];
        spu_buffer[(mLAPF1 + BufferAddress) & spu_buffer_count_mask] = Lout;
        Lout = Lout * vAPF1 + spu_buffer[(mLAPF1 + BufferAddress - dAPF1) & spu_buffer_count_mask];

        Rout -= vAPF1 * spu_buffer[(mRAPF1 + BufferAddress - dAPF1) & spu_buffer_count_mask];
        spu_buffer[(mRAPF1 + BufferAddress) & spu_buffer_count_mask] = Rout;
        Rout = Rout * vAPF1 + spu_buffer[(mRAPF1 + BufferAddress - dAPF1) & spu_buffer_count_mask];

        // Late reverb APF2
        Lout -= vAPF2 * spu_buffer[(mLAPF2 + BufferAddress - dAPF2) & spu_buffer_count_mask];
        spu_buffer[(mLAPF2 + BufferAddress) & spu_buffer_count_mask] = Lout;
        Lout = Lout * vAPF2 + spu_buffer[(mLAPF2 + BufferAddress - dAPF2) & spu_buffer_count_mask];

        Rout -= vAPF2 * spu_buffer[(mRAPF2 + BufferAddress - dAPF2) & spu_buffer_count_mask];
        spu_buffer[(mRAPF2 + BufferAddress) & spu_buffer_count_mask] = Rout;
        Rout = Rout * vAPF2 + spu_buffer[(mRAPF2 + BufferAddress - dAPF2) & spu_buffer_count_mask];

        BufferAddress = (BufferAddress + 1) & spu_buffer_count_mask;

        // Output to buffer
        leftBuffer[i] = (Lout * wet + Lin * dry) * master;
        rightBuffer[i] = (Rout * wet + Rin * dry) * master;
    }
}

void PsxVerb::setPreset(int presetIndex) {
    if (presetIndex != preset_index) {
        loadPreset(presetIndex);
    }
}

void PsxVerb::setWetGain(float newWet) {
    wet = newWet;
}

void PsxVerb::setDryGain(float newDry) {
    dry = newDry;
}

void PsxVerb::setMasterGain(float gain) {
    master = gain;
}

void PsxVerb::loadPreset(int presetIndex) {
    if (presetIndex >= NUM_PRESETS) {
        return;
    }

    float stretch_factor = rate / SPU_REV_RATE;

    PsxVerbPreset *preset = (PsxVerbPreset *)&presets[presetIndex];

    dAPF1   = (uint32_t)((preset->dAPF1 << 2) * stretch_factor);
    dAPF2   = (uint32_t)((preset->dAPF2 << 2) * stretch_factor);
    // correct 22050 Hz IIR alpha to our actual rate
    vIIR    = fc2alpha(alpha2fc(s2f(preset->vIIR), SPU_REV_RATE), rate);
    vCOMB1  = s2f(preset->vCOMB1);
    vCOMB2  = s2f(preset->vCOMB2);
    vCOMB3  = s2f(preset->vCOMB3);
    vCOMB4  = s2f(preset->vCOMB4);
    vWALL   = s2f(preset->vWALL);
    vAPF1   = s2f(preset->vAPF1);
    vAPF2   = s2f(preset->vAPF2);
    mLSAME  = (uint32_t)((preset->mLSAME << 2) * stretch_factor);
    mRSAME  = (uint32_t)((preset->mRSAME << 2) * stretch_factor);
    mLCOMB1 = (uint32_t)((preset->mLCOMB1 << 2) * stretch_factor);
    mRCOMB1 = (uint32_t)((preset->mRCOMB1 << 2) * stretch_factor);
    mLCOMB2 = (uint32_t)((preset->mLCOMB2 << 2) * stretch_factor);
    mRCOMB2 = (uint32_t)((preset->mRCOMB2 << 2) * stretch_factor);
    dLSAME  = (uint32_t)((preset->dLSAME << 2) * stretch_factor);
    dRSAME  = (uint32_t)((preset->dRSAME << 2) * stretch_factor);
    mLDIFF  = (uint32_t)((preset->mLDIFF << 2) * stretch_factor);
    mRDIFF  = (uint32_t)((preset->mRDIFF << 2) * stretch_factor);
    mLCOMB3 = (uint32_t)((preset->mLCOMB3 << 2) * stretch_factor);
    mRCOMB3 = (uint32_t)((preset->mRCOMB3 << 2) * stretch_factor);
    mLCOMB4 = (uint32_t)((preset->mLCOMB4 << 2) * stretch_factor);
    mRCOMB4 = (uint32_t)((preset->mRCOMB4 << 2) * stretch_factor);
    dLDIFF  = (uint32_t)((preset->dLDIFF << 2) * stretch_factor);
    dRDIFF  = (uint32_t)((preset->dRDIFF << 2) * stretch_factor);
    mLAPF1  = (uint32_t)((preset->mLAPF1 << 2) * stretch_factor);
    mRAPF1  = (uint32_t)((preset->mRAPF1 << 2) * stretch_factor);
    mLAPF2  = (uint32_t)((preset->mLAPF2 << 2) * stretch_factor);
    mRAPF2  = (uint32_t)((preset->mRAPF2 << 2) * stretch_factor);
    vLIN    = s2f(preset->vLIN);
    vRIN    = s2f(preset->vRIN);

    memset(spu_buffer, 0, spu_buffer_count * sizeof(float));
    preset_index = presetIndex;
}

const uint16_t PsxVerb::presets[NUM_PRESETS][0x20] = {
    {
        /* Name: Room, SPU mem required: 0x26C0 */
        0x007D, 0x005B, 0x6D80, 0x54B8, 0xBED0, 0x0000, 0x0000, 0xBA80,
        0x5800, 0x5300, 0x04D6, 0x0333, 0x03F0, 0x0227, 0x0374, 0x01EF,
        0x0334, 0x01B5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x01B4, 0x0136, 0x00B8, 0x005C, 0x8000, 0x8000,
    },
    {
        /* Name: Studio Small, SPU mem required: 0x1F40 */
        0x0033, 0x0025, 0x70F0, 0x4FA8, 0xBCE0, 0x4410, 0xC0F0, 0x9C00,
        0x5280, 0x4EC0, 0x03E4, 0x031B, 0x03A4, 0x02AF, 0x0372, 0x0266,
        0x031C, 0x025D, 0x025C, 0x018E, 0x022F, 0x0135, 0x01D2, 0x00B7,
        0x018F, 0x00B5, 0x00B4, 0x0080, 0x004C, 0x0026, 0x8000, 0x8000,
    },
    {
        /* Name: Studio Medium, SPU mem required: 0x4840 */
        0x00B1, 0x007F, 0x70F0, 0x4FA8, 0xBCE0, 0x4510, 0xBEF0, 0xB4C0,
        0x5280, 0x4EC0, 0x0904, 0x076B, 0x0824, 0x065F, 0x07A2, 0x0616,
        0x076C, 0x05ED, 0x05EC, 0x042E, 0x050F, 0x0305, 0x0462, 0x02B7,
        0x042F, 0x0265, 0x0264, 0x01B2, 0x0100, 0x0080, 0x8000, 0x8000,
    },
    {
        /* Name: Studio Large, SPU mem required: 0x6FE0*/
        0x00E3, 0x00A9, 0x6F60, 0x4FA8, 0xBCE0, 0x4510, 0xBEF0, 0xA680,
        0x5680, 0x52C0, 0x0DFB, 0x0B58, 0x0D09, 0x0A3C, 0x0BD9, 0x0973,
        0x0B59, 0x08DA, 0x08D9, 0x05E9, 0x07EC, 0x04B0, 0x06EF, 0x03D2,
        0x05EA, 0x031D, 0x031C, 0x0238, 0x0154, 0x00AA, 0x8000, 0x8000,
    },
    {
        /* Name: Hall, SPU mem required: 0xADE0 */
        0x01A5, 0x0139, 0x6000, 0x5000, 0x4C00, 0xB800, 0xBC00, 0xC000,
        0x6000, 0x5C00, 0x15BA, 0x11BB, 0x14C2, 0x10BD, 0x11BC, 0x0DC1,
        0x11C0, 0x0DC3, 0x0DC0, 0x09C1, 0x0BC4, 0x07C1, 0x0A00, 0x06CD,
        0x09C2, 0x05C1, 0x05C0, 0x041A, 0x0274, 0x013A, 0x8000, 0x8000,
    },
    {
        /* Name: Half Echo, SPU mem required: 0x3C00 */
        0x0017, 0x0013, 0x70F0, 0x4FA8, 0xBCE0, 0x4510, 0xBEF0, 0x8500,
        0x5F80, 0x54C0, 0x0371, 0x02AF, 0x02E5, 0x01DF, 0x02B0, 0x01D7,
        0x0358, 0x026A, 0x01D6, 0x011E, 0x012D, 0x00B1, 0x011F, 0x0059,
        0x01A0, 0x00E3, 0x0058, 0x0040, 0x0028, 0x0014, 0x8000, 0x8000,
    },
    {
        /* Name: Space Echo, SPU mem required: 0xF6C0 */
        0x033D, 0x0231, 0x7E00, 0x5000, 0xB400, 0xB000, 0x4C00, 0xB000,
        0x6000, 0x5400, 0x1ED6, 0x1A31, 0x1D14, 0x183B, 0x1BC2, 0x16B2,
        0x1A32, 0x15EF, 0x15EE, 0x1055, 0x1334, 0x0F2D, 0x11F6, 0x0C5D,
        0x1056, 0x0AE1, 0x0AE0, 0x07A2, 0x0464, 0x0232, 0x8000, 0x8000,
    },
    {
        /* Name: Chaos Echo, SPU mem required: 0x18040 */
        0x0001, 0x0001, 0x7FFF, 0x7FFF, 0x0000, 0x0000, 0x0000, 0x8100,
        0x0000, 0x0000, 0x1FFF, 0x0FFF, 0x1005, 0x0005, 0x0000, 0x0000,
        0x1005, 0x0005, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x1004, 0x1002, 0x0004, 0x0002, 0x8000, 0x8000,
    },
    {
        /* Name: Delay, SPU mem required: 0x18040 */
        0x0001, 0x0001, 0x7FFF, 0x7FFF, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x1FFF, 0x0FFF, 0x1005, 0x0005, 0x0000, 0x0000,
        0x1005, 0x0005, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x1004, 0x1002, 0x0004, 0x0002, 0x8000, 0x8000,
    },
    {
        /* Name: Off, SPU mem required: 0x10 */
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
        0x0000, 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
        0x0000, 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0000, 0x0000,
    }, 
};