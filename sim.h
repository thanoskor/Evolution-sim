#ifndef DNA_SIM_DNA_H
#define DNA_SIM_DNA_H
#include <stdint.h>


typedef uint64_t agent_id_t;
typedef uint32_t component_id_t;


typedef struct {
    agent_id_t number_of_agents;
    agent_id_t max_agents;
    component_id_t number_of_components;

    agent_id_t *ids;
    void **component_arrays;

} world_t;

void world_init(world_t *world, agent_id_t max_agents);

component_id_t world_add_component_type(world_t *world, size_t size);

void *world_get_component_array(world_t *world, component_id_t component_id);

#define DNA_LENGTH 64
#define LATENT_SIZE 32


typedef struct {
    unsigned char genes[DNA_LENGTH];
} dna_t;

void generate_random_dna(dna_t *dna);

#define ANTENNA_COUNT 8
typedef struct {
    float antenna_length[ANTENNA_COUNT];
    float max_speed;
    float turn_rate;
    float size;
    float max_energy;
} traits_t;

#define INPUTS 13 // 8 antenas, 1 energy, 3 state, speed
#define HIDDEN 6
#define OUTPUTS 6 // acceleration, rotational acc, attack, 3 state
#define STATE_OUTPUTS 3

typedef struct {
    float w_input_hidden[INPUTS][HIDDEN];
    float b_hidden[HIDDEN];

    float w_hidden_output[HIDDEN][OUTPUTS];
    float b_output[OUTPUTS];
} brain_t;



void nn_forward(brain_t *nn, float inputs[INPUTS], float outputs[OUTPUTS]);

typedef struct {
    Vector2 pos;
    float speed;
    float energy;
    float angle;
    float brain_state[STATE_OUTPUTS];

    traits_t traits;
    brain_t *brain;
    dna_t* dna;
} agent_t;

// if *dna is NULL a random dna will be assigned to the agent
void init_agent(agent_t *agent, float x, float y, float angle, float energy_norm, dna_t *dna);

void decode_dna(agent_t *agent);

void debug_print_agent(const agent_t *agent);

#endif //DNA_SIM_DNA_H
