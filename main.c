#define NOB_IMPLEMENTATION
#include "nob.h"
#include "raylib.h"
#include "raymath.h"

#define max(a, b) (a) > (b) ? (a) : (b)
#define min(a, b) (a) < (b) ? (a) : (b)

typedef struct
{
    Vector2 position;
    Color color;
} Food;

typedef struct
{
    Vector2 *items;
    size_t count;
    size_t capacity;
} Body;

typedef struct
{
    Body body;
    Vector2 direction;
} Snake;

#define ROWS 15
#define COLUMNS 25

static uint8_t calculate_diameter(void)
{
    float available_width = GetScreenWidth() * .90;
    float available_height = GetScreenHeight() * .90;

    float width_diameter = floor(available_width / COLUMNS);
    float height_diameter = floor(available_height / ROWS);

    return min(width_diameter, height_diameter);
}

static void draw_food(const Food *food, uint8_t diameter, Vector2 offset)
{
    Vector2 top_left_corner = Vector2Add(Vector2Scale(food->position, diameter), offset);
    Vector2 middle = {.x = top_left_corner.x + diameter / 2, .y = top_left_corner.y + diameter / 2};
    DrawCircleV(middle, diameter / 2, food->color);
}

static void draw_body(const Body *body, uint8_t diameter, Vector2 offset)
{
    nob_da_foreach(Vector2, segment, body)
    {
        Vector2 top_left_corner = Vector2Add(Vector2Scale(*segment, diameter), offset);
        Vector2 middle = {.x = top_left_corner.x + diameter / 2, .y = top_left_corner.y + diameter / 2};
        Color color = &body->items[0] == segment ? GREEN : MAGENTA;
        DrawCircleV(middle, diameter / 2, color);
    }
}

static void draw_borders(Vector2 offset)
{
    DrawRectangle(0, 0, GetScreenWidth(), offset.y, BLACK);
    DrawRectangle(0, GetScreenHeight() - offset.y, GetScreenWidth(), offset.y, BLACK);
    DrawRectangle(0, 0, offset.x, GetScreenHeight(), BLACK);
    DrawRectangle(GetScreenWidth() - offset.x, 0, offset.x, GetScreenHeight(), BLACK);
}

static void draw_score(size_t score)
{
    const char *text = nob_temp_sprintf("Score: %2lu", score);
    const size_t font_size = 20;
    Vector2 text_size = MeasureTextEx(GetFontDefault(), text, font_size, 0);
    DrawText(text,
             text_size.x / 2,            //
             text_size.y / 2, font_size, //
             DARKGRAY);
}

static const Vector2 DIRECTION_UP = (Vector2){0, -1};
static const Vector2 DIRECTION_DOWN = (Vector2){0, 1};
static const Vector2 DIRECTION_LEFT = (Vector2){-1, 0};
static const Vector2 DIRECTION_RIGHT = (Vector2){1, 0};

static bool is_opposite_direction(const Vector2 dir1, const Vector2 dir2)
{
    return (dir1.x == -dir2.x && dir1.y == dir2.y) || (dir1.x == dir2.x && dir1.y == -dir2.y);
}

typedef struct
{
    uint16_t ms_to_trigger;
    uint16_t ms_accumulated;
} Accumulator;

static bool accumulator_tick(Accumulator *accumulator, float dt)
{
    accumulator->ms_accumulated += dt * 1000.0;

    if (accumulator->ms_accumulated > accumulator->ms_to_trigger)
    {
        accumulator->ms_accumulated = 0;
        return true;
    }

    return false;
}

static void accumulator_reset(Accumulator *accumulator)
{
    accumulator->ms_accumulated = 0;
}

static bool is_food_there(const Vector2 position, const Food *food)
{
    return Vector2Equals(position, food->position);
}

static bool is_location_inside_snake(const Vector2 location, const Snake *snake)
{
    nob_da_foreach(Vector2, segment, &snake->body)
    {
        if (Vector2Equals(*segment, location))
        {
            return true;
        }
    }

    return false;
}

static Vector2 random_food_position(const Snake *snake)
{
    Vector2 return_ = {.x = 0, .y = 0};

    do
    {
        return_.x = GetRandomValue(0, COLUMNS - 1);
        return_.y = GetRandomValue(0, ROWS - 1);
    } while (is_location_inside_snake(return_, snake));

    return return_;
}

static Food food = {0};

static size_t foods_eaten = 0;

static Snake snake = {0};

static Accumulator move_timing = {
    .ms_accumulated = 0,
    .ms_to_trigger = 200,
};

static void setup(void)
{
    snake = (Snake){0};
    nob_da_append(&snake.body, ((Vector2){10, 2}));
    nob_da_append(&snake.body, ((Vector2){10, 3}));
    nob_da_append(&snake.body, ((Vector2){10, 4}));
    snake.direction = Vector2Zero();

    food = (Food){
        .position = (Vector2){1, 3},
        .color = RED,
    };

    accumulator_reset(&move_timing);
}

typedef enum
{
    Idle,
    Playing,
    Lost
} State;

static State state = Idle;

int main(void)
{
    InitWindow(800, 600, "Snake Game in Raylib");

    SetTargetFPS(60);

    RenderTexture2D target;

    setup();

    float lastHeight = 0;
    float lastWidth = 0;

    Vector2 next_direction_input = Vector2Zero();

    bool debug = true;

    while (!WindowShouldClose())
    {
        float height = GetScreenHeight();
        float width = GetScreenWidth();

        if (fabsf(height - lastHeight) > 0.001 || fabsf(width - lastWidth))
        {
            lastHeight = height;
            lastWidth = width;
            target = LoadRenderTexture(width, height);
        }

        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
        {
            next_direction_input = DIRECTION_RIGHT;
        }
        else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
        {
            next_direction_input = DIRECTION_LEFT;
        }
        else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        {
            next_direction_input = DIRECTION_UP;
        }
        else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        {
            next_direction_input = DIRECTION_DOWN;
        }

        else if (IsKeyPressed(KEY_L))
        {
            debug = !debug;
        }

        if (state == Idle || state == Lost)
        {
            snake.direction = next_direction_input;

            if (snake.direction.x != 0 || snake.direction.y != 0)
            {
                foods_eaten = 0;
                state = Playing;
            }
        }

        else if (state == Playing)
        {
            if (accumulator_tick(&move_timing, GetFrameTime()))
            {
                if ((next_direction_input.x != 0 || next_direction_input.y != 0) &&
                    (!is_opposite_direction(next_direction_input, snake.direction)))
                {
                    snake.direction = next_direction_input;
                }

                move_timing.ms_to_trigger = max(200 - (5 * (snake.body.count - 2)), 100);

                Vector2 next_head_position = snake.body.items[0];
                next_head_position = Vector2Add(next_head_position, snake.direction);

                size_t until = snake.body.count - 1;

                if (is_food_there(next_head_position, &food))
                {
                    nob_da_append(&snake.body, snake.body.items[until]);

                    food.position = random_food_position(&snake);
                    foods_eaten++;
                }

                for (size_t i = until; i > 0; i--)
                {
                    snake.body.items[i] = snake.body.items[i - 1];
                }

                snake.body.items[0] = next_head_position;

                nob_da_foreach(Vector2, part, &snake.body)
                {
                    if (part != &snake.body.items[0])
                    {
                        if (Vector2Equals(*part, snake.body.items[0]))
                        {
                            state = Lost;
                            setup();
                            goto draw;
                        }
                    }
                }

                if (next_head_position.x >= COLUMNS || next_head_position.x < 0 || next_head_position.y >= ROWS ||
                    next_head_position.y < 0)
                {
                    state = Lost;
                    setup();
                    goto draw;
                }
            }
        }

    draw:
        BeginDrawing();

        if (state == Idle || state == Lost)
        {
            BeginTextureMode(target);
        }

        ClearBackground(RAYWHITE);

        uint8_t diameter = calculate_diameter();
        float used_x = diameter * COLUMNS;
        float used_y = diameter * ROWS;
        Vector2 offset = {
            .x = (GetScreenWidth() - used_x) / 2,
            .y = (GetScreenHeight() - used_y) / 2,
        };

        if (debug)
        {
            for (size_t x = 0; x < COLUMNS; ++x)
            {
                for (size_t y = 0; y < ROWS; ++y)
                {
                    const char *text = nob_temp_sprintf("%2ld:%2ld", x, y);
                    const size_t font_size = 10;
                    Vector2 text_size = MeasureTextEx(GetFontDefault(), text, font_size, 0);
                    Vector2 where = Vector2Add(Vector2Scale(
                                                   (Vector2){
                                                       .x = x,
                                                       .y = y,
                                                   },
                                                   diameter),
                                               offset);
                    DrawRectangle(where.x, where.y, diameter, diameter, GRAY);
                    DrawText(text,
                             where.x + text_size.x / 2,            //
                             where.y + text_size.y / 2, font_size, //
                             DARKGRAY);
                }
            }
        }

        draw_borders(offset);

        draw_body(&snake.body, diameter, offset);

        draw_food(&food, diameter, offset);

        draw_score(foods_eaten);

        if (state == Idle || state == Lost)
        {
            EndTextureMode();
        }

        if (state == Idle)
        {
            DrawTextureRec(target.texture,
                           (Rectangle){0, 0, (float)target.texture.width, (float)-target.texture.height}, Vector2Zero(),
                           GRAY);

            const char *text = "Use arrow keys (or WASD) to move the snake";
            const size_t font_size = 20;
            Vector2 text_size = MeasureTextEx(GetFontDefault(), text, font_size, 0);
            DrawText(text,
                     GetScreenWidth() / 2 - text_size.x / 2,             //
                     GetScreenHeight() / 2 - text_size.y / 2, font_size, //
                     DARKGRAY);
        }

        else if (state == Lost)
        {
            DrawTextureRec(target.texture,
                           (Rectangle){0, 0, (float)target.texture.width, (float)-target.texture.height}, Vector2Zero(),
                           GRAY);

            const char *text = nob_temp_sprintf("Lost! Score: %2lu\nMove again to restart.", foods_eaten);
            const size_t font_size = 20;
            Vector2 text_size = MeasureTextEx(GetFontDefault(), text, font_size, 0);
            DrawText(text,
                     GetScreenWidth() / 2 - text_size.x / 2,             //
                     GetScreenHeight() / 2 - text_size.y / 2, font_size, //
                     DARKGRAY);
        }

        EndDrawing();

        nob_temp_reset();
    }
}
