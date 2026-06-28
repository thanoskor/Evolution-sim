#ifndef DNA_SIM_DNA_H
#define DNA_SIM_DNA_H
#include <stdint.h>
#include <raymath.h>


//dna macros
#define DNA_LENGTH 64
#define LATENT_SIZE 32

//brain macros
#define INPUTS 13 // 8 antenas, 1 energy, 3 state, speed
#define HIDDEN 6
#define OUTPUTS 6 // acceleration, rotational acc, attack, 3 state

//trait macros
#define ANTENNA_COUNT 8
#define STATE_OUTPUTS 3

//world typedefs
typedef uint64_t agent_id_t;
typedef uint32_t component_id_t;



typedef struct {
    float w_input_hidden[INPUTS][HIDDEN];
    float b_hidden[HIDDEN];

    float w_hidden_output[HIDDEN][OUTPUTS];
    float b_output[OUTPUTS];
} brain_t;


typedef struct {
    unsigned char genes[DNA_LENGTH];
} dna_t;


typedef struct {
    agent_id_t number_of_agents;
    agent_id_t max_agents;
    agent_id_t *ids;

    component_id_t number_of_components;
    void **component_arrays;
    size_t *component_sizes;

} world_t;


typedef struct {
    //transient components
    component_id_t pos;
    component_id_t speed;
    component_id_t energy;
    component_id_t angle;
    component_id_t brain_state;
    component_id_t brain_input;
    component_id_t brain_output;
    component_id_t antenna_hit_r;
    component_id_t attack_intent;

    //trait components
    component_id_t antenna_lengths;
    component_id_t max_speed;
    component_id_t turn_rate;
    component_id_t size;
    component_id_t max_energy;
    component_id_t brain;
    component_id_t dna;
} layout_t;


// sim functions
void nn_forward(brain_t *nn, float inputs[INPUTS], float outputs[OUTPUTS]);

void generate_random_dna(dna_t *dna);


float decode_max_speed(const dna_t *dna);
float decode_turn_rate(const dna_t *dna);
float decode_size(const dna_t *dna);
float decode_max_energy(const dna_t *dna);
void  decode_antenna_lengths(const dna_t *dna, float antenna_lengths[ANTENNA_COUNT]);
void  decode_brain(const dna_t *dna, brain_t *b);


void            world_init(world_t *world, agent_id_t max_agents);
component_id_t  world_add_component_type(world_t *world, size_t size);
void *          world_get_component_array(world_t *world, component_id_t component_id);
void *          world_get_component_of(world_t *world, agent_id_t index, component_id_t component_id);
agent_id_t      world_spawn_agent(world_t *world);


#endif //DNA_SIM_DNA_H
