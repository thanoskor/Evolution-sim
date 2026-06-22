#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "raylib.h"
#include "sim.h"


#define PMAX_AC 0.3f
#define DRAG_C 0.05f
#define NUERAL_STATE_DECAY 0.95f

#define MAP_RADIUS_SQR 25000000

void antenna_raycasts(agent_t *agent,
                      agent_t *near_agents,
                      int number_of_near_agents,
                      float result_array[ANTENNA_COUNT])
{

    const float dr = 1.0f;
    float cx = agent->pos.x;
    float cy = agent->pos.y;
    float start_r = agent->traits.size;

    for (int i = 0; i < ANTENNA_COUNT; i++) {
        float theta = agent->angle + (2.0f * PI * i) / ANTENNA_COUNT;
        float dx = cosf(theta);
        float dy = sinf(theta);
        float max_r = agent->traits.antenna_length[i];

        float r;
        for (r = start_r; r < max_r; r += dr) {
            float x = cx + dx * r;
            float y = cy + dy * r;
            bool hit = false;
            for (int a = 0; a < number_of_near_agents; a++) {
                // Skip self
                if (&near_agents[a] == agent) continue;

                if (Vector2DistanceSqr((Vector2){x, y}, near_agents[a].pos) <
                    near_agents[a].traits.size * near_agents[a].traits.size) {
                    hit = true;
                    break;
                    }
            }
            if (hit) break;
        }
        // Normalize: 0 = hit immediately, 1 = nothing hit at max range
        result_array[i] = r / max_r;
    }
}


bool update_agent(agent_t *agent, agent_t *near_agents, int number_of_near_agents) {

    // --------- UPDATE BRAIN ---------
    brain_t *brain = agent->brain;

    //determine the NN inputs
    int idx = 0;
    float inputs[INPUTS];


    //the first are the antenna inputs
    antenna_raycasts(agent, near_agents, number_of_near_agents, inputs);
    idx += 8;

    // next is energy
    inputs[idx++] = agent->energy / agent->traits.max_energy;

    //before hardcoding the next lines
    assert(ANTENNA_COUNT == 8);

    // these three are state numbers hardcode for now (this is bad debt)
    inputs[idx++] = agent->brain_state[0];
    inputs[idx++] = agent->brain_state[1];
    inputs[idx++] = agent->brain_state[2];

    //speed input
    inputs[idx++] = agent->speed / agent->traits.max_speed;

    assert(idx == INPUTS);

    //run the brain
    float outputs[OUTPUTS];
    nn_forward(brain, inputs, outputs);

    //update brain state with the new outputs and decay it
    agent->brain_state[0] += outputs[3];
    agent->brain_state[1] += outputs[4];
    agent->brain_state[2] += outputs[5];

    agent->brain_state[0] *= NUERAL_STATE_DECAY;
    agent->brain_state[1] *= NUERAL_STATE_DECAY;
    agent->brain_state[2] *= NUERAL_STATE_DECAY;

    agent->brain_state[0] = Clamp(agent->brain_state[0], -1.f, 1.f);
    agent->brain_state[1] = Clamp(agent->brain_state[1], -1.f, 1.f);
    agent->brain_state[2] = Clamp(agent->brain_state[2], -1.f, 1.f);


    // --------- MOVE ---------

    float acceleration = outputs[0];
    float rotational_acceleration = outputs[1];
    bool try_attack = outputs[2] > 0.f;

    // set agent speed and rotation and cap speed to be positive and under max speed
    agent->speed += acceleration * PMAX_AC * agent->traits.max_speed;
    agent->speed -= agent->speed * DRAG_C;

    if (agent->speed > agent->traits.max_speed) agent->speed = agent->traits.max_speed;
    if (agent->speed < 0.0f) agent->speed = 0.f;


    agent->angle += rotational_acceleration * agent->traits.turn_rate;


    // update pos
    // //sin and cos are slow but whatever will change later (controlls are easier that way)
    agent->pos.x += agent->speed * cosf(agent->angle);
    agent->pos.y += agent->speed * sinf(agent->angle);

    float distance_from_center = Vector2LengthSqr(agent->pos);
    if (distance_from_center - MAP_RADIUS_SQR > 0.f) {
        Vector2 vec_to_center = {-agent->pos.x, -agent->pos.y};
        //left of here
    }


    return try_attack;
}


void draw_agent_antennas(agent_t *a)
{
    float cx = a->pos.x;
    float cy = a->pos.y;

    float base_angle = a->angle;

    for (int i = 0; i < ANTENNA_COUNT; i++)
    {
        float t = (float)i / ANTENNA_COUNT;
        float ang = base_angle + t * 2.0f * PI;

        float len = a->traits.antenna_length[i];

        // start at edge of agent
        float sx = cx + cosf(ang) * a->traits.size;
        float sy = cy + sinf(ang) * a->traits.size;

        float ex = sx + cosf(ang) * len;
        float ey = sy + sinf(ang) * len;

        DrawLine(sx, sy, ex, ey, GREEN);
    }
}

int main(void)
{
    const int screenWidth = 1600;
    const int screenHeight = 900;


    // init world and spawn some agents
    world_t world;
    world_init(&world, 10000);
    world_add_component_type()

    srand(time(NULL));
    for (int i=0; i<n; i++) {
        float x = screenWidth * (rand() % 100) / 100.f;
        float y = screenHeight * (rand() % 100) / 100.f;
        init_agent(&agents[i], x , y, 0.f, 0.5f, NULL);
        // debug_print_agent(&agents[i]);
        // return 0;
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
        for (int i=0; i<n; i++) {
            a[i] = update_agent(&agents[i], agents, n);
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        for (int i=0; i<n; i++) {
            DrawCircle(agents[i].pos.x, agents[i].pos.y, agents[i].traits.size, BLACK);
            if (a[i]) DrawCircle(agents[i].pos.x, agents[i].pos.y, agents[i].traits.size * 1.5f, BLACK);
            draw_agent_antennas(&agents[i]);
        }
        EndMode2D();

        EndDrawing();
    }

    CloseWindow();

    return 0;
}

