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
    m_acceleration(0.0f),
    m_speed(0.0f),
    m_rotation(0.0f, 0.0f, 1.0f),
    m_angle(0.0f)
{ }

// Parametereized constructor
// current constructor used by the ship
Entity::Entity(GLuint texture_id, float speed, glm::vec3 acceleration) :
    m_position(0.0f),
    m_movement(0.0f),
    m_scale(1.0f, 1.0f, 1.0f),
    m_model_matrix(1.0f),
    m_velocity(0.0f),
    m_texture_id(texture_id),
    m_acceleration(acceleration),
    m_speed(speed),
    m_rotation(0.0f, 0.0f, 1.0f),
    m_angle (0.0f)
{ }


// Destructor
Entity::~Entity() {}

void Entity::update(float delta_time)
{
    // I think we don't need this
    //m_velocity.x = m_movement.x * m_speed;
    //m_velocity.y = m_movement.y * m_speed;


    // adding gravity
    m_velocity += m_acceleration * delta_time;

    m_position.y += m_velocity.y * delta_time;
    // check collision on y

    m_position.x += m_velocity.x * delta_time;
    // check collision on x

    // put the updates in
    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, m_position);
    m_model_matrix = glm::rotate(m_model_matrix, glm::radians(m_angle), m_rotation);
    m_model_matrix = glm::scale(m_model_matrix, m_scale);
}


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

void Entity::rotate(float delta_time, AngleDirection direction) 
{
    if (direction == LEFT) {
        m_angle += (delta_time * 1.0f * ANGLE_PER_TIME);
    }
    if (direction == RIGHT) {
        m_angle += (delta_time * -1.0f * ANGLE_PER_TIME);
    }
}

void Entity::updateFuel(float delta_time, bool using_fuel)
{
    // reset acceleration matrix
    m_acceleration = glm::vec3(0.0f);
    if (using_fuel) {
        m_acceleration.x = glm::cos(glm::radians(m_angle));
        m_acceleration.y = glm::sin(glm::radians(m_angle));
    }
    m_acceleration.x *= ACCEL_SCALE;
    m_acceleration.y *= ACCEL_SCALE;
    m_acceleration.y -= GRAVITY;
}


const void Entity::log_attributes() {
    std::cout << "Velocity: " << m_velocity.x << " " << m_velocity.y << std::endl;
    std::cout << "Acceleration: " << m_acceleration.x << " " << m_acceleration.y << std::endl;
    std::cout << "Position: " << m_position.x << " " << m_position.y << std::endl;
    std::cout << "Angle: " << m_angle << std::endl;
    std::cout << std::endl;
}
