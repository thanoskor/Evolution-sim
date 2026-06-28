#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "sim.h"


void world_init(world_t *world, agent_id_t max_agents) {
    world->number_of_agents = 0;
    world->max_agents = max_agents;
    world->number_of_components = 0;
    world->ids = malloc(sizeof(agent_id_t) * max_agents);
    if (!world->ids) {
        fprintf(stderr, "Failed to allocated world ids. Exiting");
        exit(1);
    }

    world->component_arrays = NULL;
    world->component_sizes = NULL;
}

//const position_id = world_add_component_type(sizeof(position_t)); #example usage
component_id_t world_add_component_type(world_t *world, size_t size) {
    // reallocate component_array pointers
    world->component_arrays = realloc(world->component_arrays, (world->number_of_components+1) * sizeof(void *));
    if (!world->component_arrays) {
        fprintf(stderr, "Failed to reallocated world component pointers. Exiting");
        exit(1);
    }

    //reallocate component_sizes
    world->component_sizes = realloc(world->component_sizes, (world->number_of_components+1) * sizeof(size_t));
    if (!world->component_sizes) {
        fprintf(stderr, "Failed to reallocate world component sizes, exiting");
        exit(1);
    }
    world->component_sizes[world->number_of_components] = size;

    //allocate component array
    world->component_arrays[world->number_of_components] = calloc(world->max_agents, size);
    if (!world->component_arrays[world->number_of_components]) {
        fprintf(stderr, "Failed to allocate component array. Exiting");
        exit(1);
    }

    return world->number_of_components++;
}


// position_t *position_array = world_get_component_array(position_id); #example usage
void *world_get_component_array(world_t *world, component_id_t component_id) {
    assert(component_id < world->number_of_components);
    return world->component_arrays[component_id];
}

void *world_get_component_of(world_t *world, agent_id_t index, component_id_t component_id) {
    assert(index < world->number_of_agents);
    assert(component_id < world->number_of_components);
    return world->component_arrays[component_id] + (index * world->component_sizes[component_id]);
}

//returns the agent index in the dense arrays (THIS CAN AND WILL CHANGE BETWEEN FRAMES)
//it opens up uninitialized memory to be read by systems so initialize immediately
agent_id_t world_spawn_agent(world_t *world) {
    if (world->number_of_agents == world->max_agents) {
        fprintf(stderr, "max agents reached not implemented growth");
        exit(1);
    }
    return world->number_of_agents++;
}
