#include <assert.h>

#include "sim.h"

#include <math.h>
#include <stdlib.h>


void generate_random_dna(dna_t *dna) {
    for (int i = 0; i < DNA_LENGTH; i++) {
        dna->genes[i] = (unsigned char)rand();
    }
}

//helpers
static inline float clamp_tanh(float x)
{
    return tanhf(x);
}

static inline float remap01(float x) {
    return x * 0.5f + 0.5f;
}

static inline float remap_range(float x01, float min, float max) {
    return min + remap01(x01) * (max - min);
}


//decoding
static float decode_latent_x(const dna_t *dna, size_t latent_index) {
    assert(LATENT_SIZE == DNA_LENGTH / 2);
    assert(latent_index < LATENT_SIZE);

    unsigned int val = (dna->genes[latent_index*2] << 8) | dna->genes[latent_index*2+1];
    return (val / 65535.0f) * 2.0f - 1.0f;
}


//decode trait functions
float decode_max_speed(const dna_t *dna) {
    return remap_range(decode_latent_x(dna, 0), 1.0f,  5.0f);
}

float decode_turn_rate(const dna_t *dna) {
    return remap_range(decode_latent_x(dna, 1), 0.01f, 0.05f);
}

float decode_size(const dna_t *dna) {
    return remap_range(decode_latent_x(dna, 2), 5.0f,  25.0f);
}

float decode_max_energy(const dna_t *dna) {
    return remap_range(decode_latent_x(dna, 2), 50.0f, 300.0f);
}

void decode_antenna_lengths(const dna_t *dna, float antenna_lengths[ANTENNA_COUNT]) {
    for (int i = 0; i < ANTENNA_COUNT; i++) {
        antenna_lengths[i] = remap_range(decode_latent_x(dna, i + 4), 0.f, 120.f);
    }
}

void decode_brain(const dna_t *dna, brain_t *b) {
    float z[16];
    for (int i = 0; i < 16; i++) {
        z[i] = decode_latent_x(dna, 16 + i);
    }
    assert(LATENT_SIZE >= 32);

    float g_inhid   = z[0];
    float g_hidout  = z[1];
    float g_bhid    = z[2];
    float g_bout    = z[3];

    float phase1 = z[4];
    float phase2 = z[5];
    float phase3 = z[6];
    float phase4 = z[7];

    // INPUT -> HIDDEN (78 weights)
    for (int i = 0; i < INPUTS; i++) {
        for (int h = 0; h < HIDDEN; h++) {

            float idx = (float)(i * HIDDEN + h);

            float v =
                g_inhid * (0.5f - idx * 0.01f) +
                0.6f * sinf(idx * phase1 + z[8]) +
                0.4f * cosf(i * z[9] + h * z[10]);

            b->w_input_hidden[i][h] = clamp_tanh(v);
        }
    }

    // HIDDEN BIAS (6)
    for (int h = 0; h < HIDDEN; h++) {

        float v =
            g_bhid +
            0.5f * sinf(h * phase2 + z[11]) +
            0.3f * z[12];

        b->b_hidden[h] = clamp_tanh(v);
    }

    // HIDDEN -> OUTPUT (36 weights)
    for (int h = 0; h < HIDDEN; h++) {
        for (int o = 0; o < OUTPUTS; o++) {

            float idx = (float)(h * OUTPUTS + o);

            float v =
                g_hidout * (0.5f - idx * 0.02f) +
                0.6f * sinf(idx * phase3 + z[8]) +
                0.4f * cosf(h * z[9] + o * z[10]);

            b->w_hidden_output[h][o] = clamp_tanh(v);
        }
    }

    // OUTPUT BIAS (6)
    for (int o = 0; o < OUTPUTS; o++) {

        float v =
            g_bout +
            0.6f * sinf(o * phase4 + z[11]) +
            0.2f * z[12];

        b->b_output[o] = clamp_tanh(v);
    }
}

