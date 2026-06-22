#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "sim.h"


//assumes dna will either be NULL in which case i create and allocate it inside or already allocated and set (by reproduction)
void init_agent(agent_t *agent, float x, float y, float angle, float energy_norm, dna_t *dna) {
    agent->pos.x = x;
    agent->pos.y = y;

    agent->speed = 0.0f;

    agent->angle = angle;
    for (int i=0; i<STATE_OUTPUTS; i++) {
        agent->brain_state[i] = 0.f;
    }

    // allocate brain, will manage differently later
    agent->brain = malloc(sizeof(brain_t));
    if (!agent->brain) {
        fprintf(stderr, "ERROR: malloc failed\n");
        exit(1);
    }


    if (!dna) {
        agent->dna = malloc(sizeof(dna_t));
        if (!agent->dna) {
            fprintf(stderr, "dna allocation failed!");
            exit(1);
        }
        generate_random_dna(agent->dna);
    }

    //this decodes dna into both the physical traits and the brain
    decode_dna(agent);

    //starting energy depends on max_energy trait so its set after decoding
    agent->energy = agent->traits.max_energy * energy_norm;

}


void debug_print_agent(const agent_t *agent) {
    printf("==================== AGENT DEBUG INFO ====================\n");

    // 1. Core State
    printf("[State]\n");
    printf("  Position: (%6.2f, %6.2f)\n", agent->pos.x, agent->pos.y);
    printf("  Angle:    %6.2f rad (%6.1f°)\n", agent->angle, agent->angle * (180.0f / PI));
    printf("  Energy:   %6.2f / %6.2f\n\n", agent->energy, agent->traits.max_energy);

    // 2. Raw DNA Sequence
    printf("[DNA Sequence]\n  ");
    for (int i = 0; i < DNA_LENGTH; i++)
    {
        printf("[%02X] ", agent->dna->genes[i]); // Prints in Hexadecimal format (e.g., FF, 0A, 8C)
    }
    printf("\n\n");

    // 3. Decoded Physical Traits
    printf("[Decoded Traits]\n");
    printf("  Max Speed:  %6.3f\n", agent->traits.max_speed);
    printf("  Turn Rate:  %6.3f\n", agent->traits.turn_rate);
    printf("  Max Size:   %6.3f\n", agent->traits.size);
    printf("  Max Energy: %6.3f\n\n", agent->traits.max_energy);

    // 4. Antenna Visualizer
    printf("[Antenna Ranges]\n");
    for (int i = 0; i < ANTENNA_COUNT; i++)
    {
        printf("  Antenna %d: %6.2f | ", i, agent->traits.antenna_length[i]);

        // Draw a simple text-based bar graph representing the length
        int bar_length = (int)(agent->traits.antenna_length[i] / 2.0f);
        for (int j = 0; j < bar_length; j++) printf("=");
        printf(">\n");
    }
    printf("==========================================================\n\n");

    // 5. Brain Debug Info
    printf("[Brain]\n\n");

    // Hidden biases
    printf("Hidden Biases (b_hidden):\n");
    for (int i = 0; i < HIDDEN; i++)
    {
        printf("  H[%d]: %10.6f\n", i, agent->brain->b_hidden[i]);
    }
    printf("\n");

    // Output biases
    printf("Output Biases (b_output):\n");
    for (int i = 0; i < OUTPUTS; i++)
    {
        printf("  O[%d]: %10.6f\n", i, agent->brain->b_output[i]);
    }
    printf("\n");

    // Input -> Hidden weights (FULL)
    printf("Weights Input -> Hidden (w_input_hidden):\n");
    for (int h = 0; h < HIDDEN; h++)
    {
        printf("  H[%d]: ", h);
        for (int i = 0; i < INPUTS; i++)
        {
            printf("%10.6f ", agent->brain->w_input_hidden[i][h]);
        }
        printf("\n");
    }
    printf("\n");

    // Hidden -> Output weights (FULL)
    printf("Weights Hidden -> Output (w_hidden_output):\n");
    for (int o = 0; o < OUTPUTS; o++)
    {
        printf("  O[%d]: ", o);
        for (int h = 0; h < HIDDEN; h++)
        {
            printf("%10.6f ", agent->brain->w_hidden_output[h][o]);
        }
        printf("\n");
    }

    printf("\n==========================================================\n\n");
}


