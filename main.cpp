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

// ----- CONSTANTS ----- //
// ----- DONT CHANGE ----- //
constexpr float WINDOW_MULTI = 1.0f;
constexpr int WINDOW_WIDTH = 640 * WINDOW_MULTI,
WINDOW_HEIGHT = 480 * WINDOW_MULTI;

constexpr float BG_RED = 1.0f,
BG_GREEN = 1.0f,
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

constexpr int FONTBANK_ROWS = 8;
constexpr int FONTBANK_COLS = 16;


// ----- OBJECT CONSTANTS ----- //
GLuint g_font_texture_id;
constexpr char SHIP_FILEPATH[] = "assets/baguette.png";
constexpr char FONTSHEET_FILEPATH[] = "assets/modified_atari_font.png";

// ----- STRUCTS AND ENUMS ----- //
enum AppStatus { RUNNING, TERMINATED };

// ----- GAME CONSTANTS ----- //

struct GameState
{
    Entity* ship;
};

// TO BE MODIFIED

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
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


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
    for (int i = 0; i < text.size(); i++) {
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
    GLuint ship_texture_id = load_texture(SHIP_FILEPATH);
    g_font_texture_id = load_texture(FONTSHEET_FILEPATH);


    // STUFF TO INITIALISE //
    g_game_state.ship = new Entity(
        ship_texture_id,                    // texture_id
        5.0f,                               // speed
        glm::vec3(0.0f, 0.0f, 0.0f)        // acceleration vector
    );
    g_game_state.ship->set_movement(glm::vec3(0.0f, 0.0f, 0.0f));


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
            ;
        }
        
        // update acceleration and fuel usage
        g_game_state.ship->updateFuel(FIXED_TIMESTEP, g_using_fuel);

        // update the position after all of that
        g_game_state.ship->update(FIXED_TIMESTEP);

        // decrement
        delta_time -= FIXED_TIMESTEP;
        g_game_state.ship->log_attributes();
    }

    g_accumulator = delta_time;
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // render text
    draw_text(&g_shader_program, g_font_texture_id, "FUEL", 0.25f, 0.05f,
        glm::vec3(-3.5f, 2.0f, 0.0f));

    // THINGS TO RENDER //
    g_game_state.ship->render(&g_shader_program);

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