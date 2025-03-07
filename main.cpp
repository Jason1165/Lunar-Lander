#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"

// ----- SOURCES ----- //
// Atari font sheet, adapated for this project: https://en.wikipedia.org/wiki/Atari_ST_character_set
// Ship in a Bottle (SHIP) by Laura Brown, adapted for this project: https://asciiartist.com/ldb/transportationtravelascii.txt
// Asciiquarium, inspiration for the assets: https://github.com/cmatsuoka/asciiquarium
// Underwater Castle, taken from asciiquarium, modified for better ratios and hitboxes, credit goes to Joan Stark ???


// ----- CONSTANTS ----- //
// ----- DONT CHANGE ----- //
constexpr float WINDOW_MULTI = 1.5f;
constexpr int WINDOW_WIDTH = 640 * WINDOW_MULTI,
WINDOW_HEIGHT = 480 * WINDOW_MULTI;

constexpr float BG_RED = 0.0f,
BG_GREEN = 0.0f,
BG_BLUE = 1.0f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1,
LEVEL_OF_DETAIL = 0,
TEXTURE_BORDER = 0;


// ----- OBJECT CONSTANTS ----- //
GLuint g_font_texture_id;
constexpr char SHIP_FILEPATH[] = "assets/bottle_ship_flip.png"; // 208 * 96 13:6
constexpr char FONTSHEET_FILEPATH[] = "assets/modified_atari_font.png"; // 256 * 256 
constexpr char PLATFORM1_FILEPATH[] = "assets/castle.png"; // 256 * 128

// ----- STRUCTS AND ENUMS ----- //
enum AppStatus { RUNNING, TERMINATED };
enum FilterType {NEAREST, LINEAR }; // trying to fix the glitchy rendering but whatever

struct GameState
{
    Entity* ship;
    Entity* platforms;
};

// ----- GAME CONSTANTS ----- //
constexpr int FONTBANK_ROWS = 8;
constexpr int FONTBANK_COLS = 16;
constexpr int NUM_PLATFORMS = 1;

// ----- VARIABLES ----- //
GameState g_game_state;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
AngleDirection g_angle_dir = NONE;
bool g_using_fuel = false;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

void initialise();
void process_input();
void update();
void render();
void shutdown();

GLuint load_texture(const char* filepath);



// ---- GENERAL FUNCTIONS ---- //
GLuint load_texture(const char* filepath, FilterType filterType)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components,
        STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER,
        GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        filterType == NEAREST ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
        filterType == NEAREST ? GL_NEAREST : GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void draw_text(ShaderProgram* shader_program, GLuint font_texture_id, std::string text,
    float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_COLS;
    float height = 1.0f / FONTBANK_ROWS;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for
    // each character. Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (size_t i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their
        //    position relative to the whole sentence)
        int spritesheet_index = (int)text[i];  // ascii value of character
        float offset = (font_size + spacing) * i;

        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float)(spritesheet_index % FONTBANK_COLS) / FONTBANK_COLS;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_COLS) / FONTBANK_ROWS;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
            });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    // custom scale to preserve original dimensions
    model_matrix = glm::scale(model_matrix, glm::vec3(0.5f, 1.0f, 1.0f));

    shader_program->set_model_matrix(model_matrix);
    glUseProgram(shader_program->get_program_id());

    glVertexAttribPointer(shader_program->get_position_attribute(), 2, GL_FLOAT, false, 0,
        vertices.data());
    glEnableVertexAttribArray(shader_program->get_position_attribute());

    glVertexAttribPointer(shader_program->get_tex_coordinate_attribute(), 2, GL_FLOAT,
        false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(shader_program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(shader_program->get_position_attribute());
    glDisableVertexAttribArray(shader_program->get_tex_coordinate_attribute());
}


void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Lunar Lander!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (context == nullptr)
    {
        LOG("ERROR: Could not create OpenGL context.\n");
        shutdown();
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);

    // TEXTURES 
    GLuint ship_texture_id = load_texture(SHIP_FILEPATH, NEAREST);
    GLuint castle_texture_id = load_texture(PLATFORM1_FILEPATH, NEAREST);
    g_font_texture_id = load_texture(FONTSHEET_FILEPATH, NEAREST);

    // ----- STUFF TO INITIALISE ----- //

    // ----- SHIP ----- //
    g_game_state.ship = new Entity(
        ship_texture_id,                    // texture_id
        5.0f,                               // speed
        glm::vec3(0.0f, 0.0f, 0.0f),        // acceleration vector
        true,                               // use acceleration
        START                               // EntityStatus
    );
    g_game_state.ship->set_movement(glm::vec3(0.0f, 0.0f, 0.0f));
    g_game_state.ship->set_scale(glm::vec3(1.0833f, 0.5f, 1.0f));
    g_game_state.ship->set_position(glm::vec3(-4.5f, 3.5f, 1.0f));
    g_game_state.ship->set_dimensions(g_game_state.ship->get_scale().x, g_game_state.ship->get_scale().y);
    g_game_state.ship->update(0.0f, nullptr, 0);


    // ----- PLATFORMS ----- //
    g_game_state.platforms = new Entity[NUM_PLATFORMS];
    g_game_state.platforms[0] = Entity(castle_texture_id, 0.0f, glm::vec3(0.0f), false, ACTIVE);
    g_game_state.platforms[0].set_position(glm::vec3(4.0f, -3.25f, 1.0f));
    g_game_state.platforms[0].set_scale(glm::vec3(2.0f, 1.0f, 1.0f));

    for (int i = 0; i < NUM_PLATFORMS; i++)
    {
        g_game_state.platforms[i].update(0.0f, nullptr, 0);
        g_game_state.platforms[i].set_dimensions(g_game_state.platforms[i].get_scale().x, g_game_state.platforms[i].get_scale().y);
    }

    // ----- GENERAL ----- //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    // reset 
    g_angle_dir = NONE;
    g_using_fuel = false;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_q:
                g_app_status = TERMINATED;
                break;
            case SDLK_SPACE:
                if (g_game_state.ship->get_status() == START) {
                    g_game_state.ship->set_status(ACTIVE);
                }
                break;
            }
        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);
    if (key_state[SDL_SCANCODE_LEFT]) {
        g_angle_dir = LEFT;
    }
    else if (key_state[SDL_SCANCODE_RIGHT]) {
        g_angle_dir = RIGHT;
    }
    else if (key_state[SDL_SCANCODE_UP]) {
        g_using_fuel = true;
    }

}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP)
    {
        // update angle first
        if (g_angle_dir != NONE) {
            g_game_state.ship->rotate(FIXED_TIMESTEP, g_angle_dir);
        }
        
        // update acceleration and fuel usage
        g_game_state.ship->update_fuel(FIXED_TIMESTEP, g_using_fuel);

        // update the position after all of that
        g_game_state.ship->update(FIXED_TIMESTEP, g_game_state.platforms, NUM_PLATFORMS);

        // decrement
        delta_time -= FIXED_TIMESTEP;
        //g_game_state.ship->log_attributes();
    }

    g_accumulator = delta_time;
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    std::string fuel_string = "FUEL: " + std::to_string(g_game_state.ship->get_fuel());
    glm::vec3 curr_velocity = g_game_state.ship->get_velocity();
    std::string x_velocity = "X_SPEED " + std::to_string(int(curr_velocity.x*100));
    std::string y_velocity = "Y_SPEED: " + std::to_string(int(curr_velocity.y*100));
    std::string angle_str = "ANGLE: " + std::to_string(int(g_game_state.ship->get_angle()));

    // render text
    draw_text(&g_shader_program, g_font_texture_id, fuel_string, 0.25f, 0.05f, glm::vec3(3.0f, 3.5f, 0.0f));
    draw_text(&g_shader_program, g_font_texture_id, x_velocity, 0.25f, 0.05f, glm::vec3(3.0f, 3.25f, 0.0f));
    draw_text(&g_shader_program, g_font_texture_id, y_velocity, 0.25f, 0.05f, glm::vec3(3.0f, 3.0f, 0.0f));
    draw_text(&g_shader_program, g_font_texture_id, angle_str, 0.25f, 0.05f, glm::vec3(3.0f, 2.75f, 0.0f));


    // THINGS TO RENDER //
    g_game_state.ship->render(&g_shader_program);
    for (int i = 0; i < NUM_PLATFORMS; i++) 
    {
        g_game_state.platforms[i].render(&g_shader_program);
    }

    if (g_game_state.ship->get_status() == START)
    {
        draw_text(&g_shader_program, g_font_texture_id, "PRESS SPACE TO BEGIN", 0.25f, 0.05f, glm::vec3(-1.2f, 0.0f, 0.0f));
    }


    SDL_GL_SwapWindow(g_display_window);
}


void shutdown()
{
    SDL_Quit();

    // delete pointers
}

// ----- GAME LOOP ----- //
int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}