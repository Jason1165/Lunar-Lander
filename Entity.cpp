#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'


#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Entity.h"

// Default constructor
Entity::Entity() :
	m_position(0.0f),
	m_movement(0.0f),
	m_scale(1.0f, 1.0f, 1.0f),
	m_model_matrix(1.0f),
	m_velocity(0.0f),
	m_acceleration(0.0f)
{
	// some code for later use?
}

// Parametereized constructor
// to be updated as things get complicated
Entity::Entity(GLuint texture_id) :
	m_texture_id(texture_id) 
{
	Entity(); // calling default constructor cause im lazy
}

// Destructor
Entity::~Entity() { }

void Entity::render(ShaderProgram* program)
{
    program->set_model_matrix(m_model_matrix);

    //if (m_animation_indices != NULL)
    //{
    //    draw_sprite_from_texture_atlas(program, m_texture_id, m_animation_indices[m_animation_index]);
    //    return;
    //}

    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float tex_coords[] = { 0.0,  1.0, 1.0,  1.0, 1.0, 0.0,  0.0,  1.0, 1.0, 0.0,  0.0, 0.0 };

    glBindTexture(GL_TEXTURE_2D, m_texture_id);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}



