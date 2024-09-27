/**
* Author: Jia Huang
* Assignment: Simple 2D Scene
* Date due: 2024-09-28, 11:58pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 640 * 2,
              WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED     = 0.749f,
                BG_GREEN   = 0.914f,
                BG_BLUE    = 1.0f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
                LEVEL_OF_DETAIL    = 0, // mipmap reduction image level
                TEXTURE_BORDER     = 0; // this value MUST be zero

constexpr char SUN_SPRITE_FILEPATH[]    = "sun.png",
               BIRD_SPRITE_FILEPATH[]   = "bird.png",
               FALCON_SPRITE_FILEPATH[] = "falcon.png",
               SKY_SPRITE_FILEPATH[]    = "sky.jpg",
               BLOOD_SPRITE_FILEPATH[]  = "blood.png";

constexpr glm::vec3 INIT_SCALE_SUN    = glm::vec3(2.0f, 2.0f, 0.0f),
                    INIT_SCALE_SKY    = glm::vec3(10.0f, 10.0f, 0.0f),
                    INIT_SCALE_FALCON = glm::vec3(2.0f, 1.5f, 0.0f),
                    INIT_POS_SUN      = glm::vec3(-3.0f, 2.25f, 0.0f),
                    INIT_POS_BIRD     = glm::vec3(5.0f, -1.0f, 0.0f),
                    INIT_POS_FALCON   = glm::vec3(0.0f, 0.0f, 0.0f),
                    INIT_POS_SKY      = glm::vec3(0.0f, 1.0f, 0.0f),
                    INIT_POS_BLOOD    = glm::vec3(0.0f, 0.0f, 0.0f);

constexpr float ROT_INCREMENT = 1.0f;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
          g_sun_matrix,
          g_bird_matrix,
          g_falcon_matrix,
          g_sky_matrix,
          g_blood_matrix,
          g_projection_matrix;

float g_previous_ticks = 0.0f;

glm::vec3 g_rotation_sun     = glm::vec3(0.0f, 0.0f, 0.0f),
          g_translate_bird   = glm::vec3(0.0f, 0.0f, 0.0f),
          g_translate_falcon = glm::vec3(2.5f, 2.5f, 0.0f),
          g_scale_blood      = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint g_sun_texture_id,
       g_bird_texture_id,
       g_falcon_texture_id,
       g_sky_texture_id,
       g_blood_texture_id;

float theta = 0.0f;

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}


void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("The Circle of Life.",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_sun_matrix        = glm::mat4(1.0f);
    g_bird_matrix       = glm::mat4(1.0f);
    g_falcon_matrix     = glm::mat4(1.0f);
    g_sky_matrix        = glm::mat4(1.0f);
    g_blood_matrix      = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_sun_texture_id    = load_texture(SUN_SPRITE_FILEPATH);
    g_bird_texture_id   = load_texture(BIRD_SPRITE_FILEPATH);
    g_falcon_texture_id = load_texture(FALCON_SPRITE_FILEPATH);
    g_sky_texture_id    = load_texture(SKY_SPRITE_FILEPATH);
    g_blood_texture_id  = load_texture(BLOOD_SPRITE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}


void update()
{
    /* Delta time calculations */
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    /* Game logic */
    g_rotation_sun.z += 0.25f * ROT_INCREMENT * delta_time;
    
    g_translate_bird.x += -0.5f * delta_time;
    float frequency = 1.5f;
    float amplitude = 0.5f;
    theta += frequency * delta_time;
    g_translate_bird.y = amplitude * sin(theta);
    
    if (g_translate_bird.x <= -9.0f) {
        g_scale_blood.x += 0.5f * delta_time;
        g_scale_blood.y += 0.5f * delta_time;
    }
    
    if (g_translate_bird.x <= -12.5f) {
        g_translate_bird.x = 0.0f;
        g_translate_falcon.x = 2.5f;
        g_translate_falcon.y = 2.5f;
        g_scale_blood.x = 0.0f;
        g_scale_blood.y = 0.0f;
    }
    
    g_translate_falcon.x -= 0.1f * delta_time;
    g_translate_falcon.y -= 0.1f * delta_time;

    /* Model matrix reset */
    g_sky_matrix    = glm::mat4(1.0f);
    g_sun_matrix    = glm::mat4(1.0f);
    g_bird_matrix   = glm::mat4(1.0f);
    g_falcon_matrix = glm::mat4(1.0f);
    g_blood_matrix  = glm::mat4(1.0f);

    /* Transformations */
    g_sky_matrix = glm::translate(g_sky_matrix, INIT_POS_SKY);
    g_sky_matrix = glm::scale(g_sky_matrix, INIT_SCALE_SKY);
    
    g_sun_matrix = glm::translate(g_sun_matrix, INIT_POS_SUN);
    g_sun_matrix = glm::rotate(g_sun_matrix,
                               g_rotation_sun.z,
                               glm::vec3(0.0f, 0.0f, 1.0f));
    g_sun_matrix = glm::scale(g_sun_matrix, INIT_SCALE_SUN);
    
    g_bird_matrix = glm::translate(g_bird_matrix, INIT_POS_BIRD);
    g_bird_matrix = glm::translate(g_bird_matrix, g_translate_bird);
    
    g_falcon_matrix = glm::translate(g_falcon_matrix, INIT_POS_FALCON);
    g_falcon_matrix = glm::translate(g_bird_matrix, g_translate_falcon);
    g_falcon_matrix = glm::scale(g_falcon_matrix, INIT_SCALE_FALCON);
    
    g_blood_matrix = glm::translate(g_bird_matrix, INIT_POS_BLOOD);
    g_blood_matrix = glm::scale(g_blood_matrix, g_scale_blood);
}


void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
                          0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_sky_matrix, g_sky_texture_id);
    draw_object(g_sun_matrix, g_sun_texture_id);
    draw_object(g_bird_matrix, g_bird_texture_id);
    draw_object(g_falcon_matrix, g_falcon_texture_id);
    draw_object(g_blood_matrix, g_blood_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


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
