#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "raylib.h"
#include "sim.h"
#include "external/cgltf.h"


#define PMAX_AC 0.3f
#define DRAG_C 0.05f
#define NUERAL_STATE_DECAY 0.95f
#define MAP_RADIUS_SQR 25000000


void system_antenna(world_t *w, const layout_t *l) {
    const float dr = 1.0f;

    Vector2 *pos = world_get_component_array(w, l->pos);
    float *angle = world_get_component_array(w, l->angle);
    float *size = world_get_component_array(w, l->size);
    float (*antenna_hit_r)[ANTENNA_COUNT] = world_get_component_array(w, l->antenna_hit_r);
    float (*antenna_lengths)[ANTENNA_COUNT] = world_get_component_array(w, l->antenna_lengths);

    agent_id_t n = w->number_of_agents;
    for (agent_id_t i = 0; i < n; i++) {

        float cx = pos[i].x;
        float cy = pos[i].y;
        float start_r = size[i];

        for (int a = 0; a < ANTENNA_COUNT; a++) {

            float theta = angle[i] + (2.0f * PI * a) / ANTENNA_COUNT;
            float dx = cosf(theta);
            float dy = sinf(theta);

            float max_r = antenna_lengths[i][a];

            float r;

            for (r = start_r; r < max_r; r += dr) {

                float x = cx + dx * r;
                float y = cy + dy * r;

                bool hit = false;

                for (agent_id_t j = 0; j < n; j++) {
                    if (j == i) continue;

                    float dx2 = x - pos[j].x;
                    float dy2 = y - pos[j].y;

                    float rr = size[j];

                    if (dx2 * dx2 + dy2 * dy2 < rr * rr) {
                        hit = true;
                        break;
                    }
                }

                if (hit) break;
            }

            antenna_hit_r[i][a] = r / max_r;
        }
    }
}

void system_sense(world_t *w, const layout_t *l) {
    float *speed = world_get_component_array(w, l->speed);
    float *energy = world_get_component_array(w, l->energy);
    float *max_energy = world_get_component_array(w, l->max_energy);
    float (*brain_state)[STATE_OUTPUTS] = world_get_component_array(w, l->brain_state);
    float (*inputs)[INPUTS] = world_get_component_array(w, l->brain_input);
    float (*antenna_hit_r)[ANTENNA_COUNT] = world_get_component_array(w, l->antenna_hit_r);

    agent_id_t n = w->number_of_agents;
    for (agent_id_t i = 0; i < n; i++) {

        int idx = 0;

        // antenna perception already computed elsewhere OR inline here
        for (int a = 0; a < ANTENNA_COUNT; a++) {
            inputs[i][idx++] = antenna_hit_r[i][a];
        }

        inputs[i][idx++] = energy[i] / max_energy[i];

        inputs[i][idx++] = brain_state[i][0];
        inputs[i][idx++] = brain_state[i][1];
        inputs[i][idx++] = brain_state[i][2];

        inputs[i][idx++] = speed[i]; // assume normalized or normalize here
    }
}

void system_brain(world_t *w, const layout_t *l) {
    brain_t *brain =
        world_get_component_array(w, l->brain);

    float (*inputs)[INPUTS] =
        world_get_component_array(w, l->brain_input);

    float (*outputs)[OUTPUTS] =
        world_get_component_array(w, l->brain_output);

    float (*state)[STATE_OUTPUTS] =
        world_get_component_array(w, l->brain_state);

    agent_id_t n = w->number_of_agents;

    for (agent_id_t i = 0; i < n; i++) {

        nn_forward(&brain[i], inputs[i], outputs[i]);

        // state update could revise logic later
        for (int s = 0; s < 3; s++) {
            state[i][s] += outputs[i][3 + s];
            state[i][s] *= NUERAL_STATE_DECAY;

            if (state[i][s] > 1.f) state[i][s] = 1.f;
            if (state[i][s] < -1.f) state[i][s] = -1.f;
        }
    }
}

void system_move(world_t *w, const layout_t *l) {
    Vector2 *pos = world_get_component_array(w, l->pos);
    float *angle = world_get_component_array(w, l->angle);
    float *speed = world_get_component_array(w, l->speed);

    float *max_speed = world_get_component_array(w, l->max_speed);
    float *turn_rate = world_get_component_array(w, l->turn_rate);

    float (*outputs)[OUTPUTS] =
        world_get_component_array(w, l->brain_output);

    agent_id_t n = w->number_of_agents;

    for (agent_id_t i = 0; i < n; i++) {

        float accel = outputs[i][0];
        float rot = outputs[i][1];

        speed[i] += accel * PMAX_AC * max_speed[i];
        speed[i] -= speed[i] * DRAG_C;

        if (speed[i] < 0) speed[i] = 0;
        if (speed[i] > max_speed[i]) speed[i] = max_speed[i];

        angle[i] += rot * turn_rate[i];

        pos[i].x += speed[i] * cosf(angle[i]);
        pos[i].y += speed[i] * sinf(angle[i]);
    }
}

void system_attack(world_t *w, layout_t *l)
{
    float (*outputs)[OUTPUTS] =
        world_get_component_array(w, l->brain_output);

    float *attack_intent =
        world_get_component_array(w, l->attack_intent);

    agent_id_t n = w->number_of_agents;

    for (agent_id_t i = 0; i < n; i++) {
        attack_intent[i] = outputs[i][2] > 0.f ? 1 : 0;
    }
}

void draw_agent_antennas(Vector2 pos,
                         float angle,
                         float size,
                         float antenna_lengths[ANTENNA_COUNT])
{
    for (int i = 0; i < ANTENNA_COUNT; i++) {

        float theta = angle + (2.0f * PI * i) / ANTENNA_COUNT;

        float x2 = pos.x + cosf(theta) * (size + antenna_lengths[i]);
        float y2 = pos.y + sinf(theta) * (size + antenna_lengths[i]);

        DrawLine(
            pos.x,
            pos.y,
            x2,
            y2,
            BLACK
        );
    }
}

void draw_agents(world_t *w, const layout_t *layout)
{
    Vector2 *pos =
        world_get_component_array(w, layout->pos);

    float *size =
        world_get_component_array(w, layout->size);

    float *angle =
        world_get_component_array(w, layout->angle);

    float (*antenna_lengths)[ANTENNA_COUNT] =
        world_get_component_array(w, layout->antenna_lengths);

    float *attack_intent =
        world_get_component_array(w, layout->attack_intent);


    for (agent_id_t i = 0; i < w->number_of_agents; i++) {

        DrawCircle(
            pos[i].x,
            pos[i].y,
            size[i],
            BLACK
        );

        if (attack_intent[i] > 0.0f) {
            DrawCircle(
                pos[i].x,
                pos[i].y,
                size[i] * 1.5f,
                BLACK
            );
        }

        draw_agent_antennas(
            pos[i],
            angle[i],
            size[i],
            antenna_lengths[i]
        );
    }
}



int main(void)
{
    const int screenWidth = 1600;
    const int screenHeight = 900;


    // init world and add components
    world_t world;
    world_t *w = &world;
    world_init(w, 1000);

    //layout decleration
    layout_t layout;

    //transient components
    layout.pos = world_add_component_type(w, sizeof(Vector2));
    layout.speed = world_add_component_type(w, sizeof(float));
    layout.energy = world_add_component_type(w, sizeof(float));
    layout.angle = world_add_component_type(w, sizeof(float));
    layout.brain_state = world_add_component_type(w, sizeof(float) * STATE_OUTPUTS);

    layout.brain_input = world_add_component_type(w, sizeof(float) * INPUTS);
    layout.brain_output = world_add_component_type(w, sizeof(float) * OUTPUTS);
    layout.antenna_hit_r = world_add_component_type(w, sizeof(float) * ANTENNA_COUNT);
    layout.attack_intent = world_add_component_type(w, sizeof(bool));

    layout.antenna_lengths = world_add_component_type(w, sizeof(float) * ANTENNA_COUNT);
    layout.max_speed = world_add_component_type(w, sizeof(float));
    layout.turn_rate = world_add_component_type(w, sizeof(float));
    layout.size = world_add_component_type(w, sizeof(float));
    layout.max_energy = world_add_component_type(w, sizeof(float));
    layout.brain = world_add_component_type(w, sizeof(brain_t));

    layout.dna = world_add_component_type(w, sizeof(dna_t));


    //spawn agents
    srand(time(NULL));
    int n = 10;
    for (int i=0; i<n; i++) {
        float x = screenWidth * (rand() % 100) / 100.f;
        float y = screenHeight * (rand() % 100) / 100.f;

        agent_id_t ai =  world_spawn_agent(w);
        ((Vector2*)world_get_component_of(w, ai, layout.pos))->x = x;
        ((Vector2*)world_get_component_of(w, ai, layout.pos))->y = y;

        *(float*)world_get_component_of(w, ai, layout.speed) = 0.f;
        *(bool*)world_get_component_of(w, ai, layout.attack_intent) = false;

        *(float*)world_get_component_of(w, ai, layout.angle) = 0.f;
        memset(world_get_component_of(w, ai, layout.brain_state), 0, sizeof(float) * STATE_OUTPUTS); //intialize brain state to 0.f
        memset(world_get_component_of(w, ai, layout.brain_input), 0, sizeof(float) * INPUTS); // ~ inputs
        memset(world_get_component_of(w, ai, layout.brain_output), 0, sizeof(float) * OUTPUTS); // ~ outputs
        memset(world_get_component_of(w, ai, layout.antenna_hit_r), 0, sizeof(float) * ANTENNA_COUNT); // ~antenna hit

        dna_t *dna = world_get_component_of(w, ai, layout.dna);
        generate_random_dna(dna);

        decode_antenna_lengths(dna, world_get_component_of(w, ai, layout.antenna_lengths));
        decode_brain(dna, world_get_component_of(w, ai, layout.brain));

        *(float*)world_get_component_of(w, ai, layout.max_energy) = decode_max_energy(dna);
        *(float*)world_get_component_of(w, ai, layout.max_speed) = decode_max_speed(dna);
        *(float*)world_get_component_of(w, ai, layout.size) = decode_size(dna);
        *(float*)world_get_component_of(w, ai, layout.turn_rate) = decode_turn_rate(dna);

        *(float*)world_get_component_of(w, ai, layout.energy) = decode_max_energy(dna) * 0.5f; // redecoding energy just because
        //initialization is verbose (fix it later)
    }


    // window stuff
    SetTraceLogLevel(LOG_WARNING); // for raylib to not spam the console
    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");
    SetTargetFPS(60);

    //camera stuff
    Camera2D camera = { 0 };
    camera.target = (Vector2){ 0, 0 };
    camera.offset = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    float moveSpeed = 300.0f;
    float zoomSpeed = 0.1f;

    bool a[n];
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();


        // -------- Camera Movement (WASD / Arrow Keys) --------
        Vector2 move = { 0 };

        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    move.y -= 1;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  move.y += 1;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  move.x -= 1;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move.x += 1;

        camera.target.x += move.x * moveSpeed * dt;
        camera.target.y += move.y * moveSpeed * dt;

        // -------- Zoom (Mouse Wheel) --------
        float wheel = GetMouseWheelMove();
        if (wheel != 0)
        {
            camera.zoom += wheel * zoomSpeed;
            if (camera.zoom < 0.2f) camera.zoom = 0.2f;
            if (camera.zoom > 5.0f) camera.zoom = 5.0f;
        }



        // Update
        system_antenna(w, &layout);
        system_sense(w, &layout);
        system_brain(w, &layout);
        system_move(w, &layout);
        system_attack(w, &layout);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        draw_agents(w, &layout);

        EndMode2D();

        EndDrawing();
    }

    CloseWindow();

    return 0;
}

