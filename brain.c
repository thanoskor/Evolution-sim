#include <assert.h>
#include <math.h>
#include "sim.h"

void nn_forward(brain_t *nn, float inputs[INPUTS], float outputs[OUTPUTS]) {
    float hidden[HIDDEN];

    assert(nn);
    // Input -> Hidden
    for (int h = 0; h < HIDDEN; h++)
    {
        float sum = nn->b_hidden[h];

        for (int i = 0; i < INPUTS; i++)
        {
            sum += inputs[i] * nn->w_input_hidden[i][h];
        }

        hidden[h] = tanhf(sum);
    }

    // Hidden -> Output
    for (int o = 0; o < OUTPUTS; o++)
    {
        float sum = nn->b_output[o];

        for (int h = 0; h < HIDDEN; h++)
        {
            sum += hidden[h] * nn->w_hidden_output[h][o];
        }

        outputs[o] = tanhf(sum);
    }
}
