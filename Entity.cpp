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

#include <vector>

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
    m_angle(0.0f),
    m_fuel(500),
    m_width(0.0f),
    m_height(0.0f),
    m_use_acceleration(false),
    m_status(START)
{ }

// Parametereized constructor
// current constructor used by the ship
Entity::Entity(GLuint texture_id, float speed, glm::vec3 acceleration, bool use_accel, EntityStatus status) :
    m_position(0.0f),
    m_movement(0.0f),
    m_scale(1.0f, 1.0f, 1.0f),
    m_model_matrix(1.0f),
    m_velocity(0.0f),
    m_texture_id(texture_id),
    m_acceleration(acceleration),
    m_speed(speed),
    m_rotation(0.0f, 0.0f, 1.0f),
    m_angle(0.0f),
    m_fuel(500),
    m_width(0.0f),
    m_height(0.0f),
    m_use_acceleration(use_accel),
    m_status(status)
{ }


// Destructor
Entity::~Entity() {}

void Entity::update(float delta_time, Entity* collidable_entities, int collidable_entity_count)
{
    // check for in bounds
    std::pair<float, float> x_coors = this->get_min_max_x();
    std::pair<float, float> y_coors = this->get_min_max_y();

    if (x_coors.second > 5.0f || x_coors.first < -5.0f)
    {
        this->set_status(CRASHED);
    }
    if (y_coors.first < -3.75f)
    {
        this->set_status(CRASHED);
    }


    // check for collision
    for (int i = 0; i < collidable_entity_count; i++)
    {
        if (check_collision_SAT(&collidable_entities[i])) {
            m_velocity = glm::vec3(0.0f); // pause all velocities
            valid_collision(&collidable_entities[i]);
            return;
        }
    }

    // we playing?
    if (m_status == ACTIVE && !m_use_acceleration) {
        m_velocity.x = m_movement.x * m_speed;
        m_velocity.y = m_movement.y * m_speed;
    }
    if (m_status == ACTIVE && m_use_acceleration) {
        // adding gravity
        m_velocity += m_acceleration * delta_time;

        m_position.y += m_velocity.y * delta_time;

        m_position.x += m_velocity.x * delta_time;
    }

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

void Entity::update_fuel(float delta_time, bool using_fuel)
{
    // reset acceleration matrix
    m_acceleration = glm::vec3(0.0f);
    if (using_fuel && m_fuel > 0) {
        m_acceleration.x = glm::cos(glm::radians(m_angle));
        m_acceleration.y = glm::sin(glm::radians(m_angle));
        m_fuel -= FUEL_PER_TIME;
    }
    m_acceleration.x *= ACCEL_SCALE;
    m_acceleration.y *= ACCEL_SCALE;
    m_acceleration.y -= GRAVITY;
}


void Entity::set_dimensions(float x, float y) {
    m_height = y;
    m_width = x;
}

// ----- COLLISION STUFF ----- //

std::vector<glm::vec2> Entity::get_corners()
{
    std::vector<glm::vec2> corners;
    float half_width = m_width / 2.0f;
    float half_height = m_height / 2.0f;

    std::vector<glm::vec2> local_corners = {
        {-half_width,  half_height},        // Top-left
        { half_width,  half_height},        // Top-right
        { half_width, -half_height},        // Bottom-right
        {-half_width, -half_height}         // Bottom-left
    };

    float angle_rad = glm::radians(m_angle);
    float cos_theta = glm::cos(angle_rad);
    float sin_theta = glm::sin(angle_rad);

    for (auto& vertex : local_corners)
    {
        float local_x = vertex.x;
        float local_y = vertex.y;

        float rotated_x = cos_theta * local_x - sin_theta * local_y;
        float rotated_y = sin_theta * local_x + cos_theta * local_y;

        corners.push_back(glm::vec2(m_position.x + rotated_x, m_position.y + rotated_y));
    }

    return corners;
}

std::vector<glm::vec2> Entity::get_edges()
{
    std::vector<glm::vec2> corners = get_corners();
    std::vector<glm::vec2> edges;

    for (size_t i = 0; i < corners.size(); i++)
    {
        edges.push_back(corners[(i + 1) % corners.size()] - corners[i]);
    }

    return edges;
}

std::vector<glm::vec2> Entity::get_normals()
{
    std::vector<glm::vec2> edges = get_edges();
    std::vector<glm::vec2> normals;

    for (auto& edge : edges)
    {
        normals.push_back(glm::vec2(-edge.y, edge.x));  
    }

    // Normalize all the normals
    for (auto& normal : normals)
    {
        normal = glm::normalize(normal);
    }

    return normals;
}

bool Entity::check_collision_SAT(Entity* other)
{
    // get the entity corners to project onto the axes
    std::vector<glm::vec2> self_corners = this->get_corners();
    std::vector<glm::vec2> other_corners = other->get_corners();

    // get the axes
    std::vector<glm::vec2> self_normals = this->get_normals();
    std::vector<glm::vec2> other_normals = other->get_normals();

    // append axes to one list
    std::vector<glm::vec2> axes;
    axes.insert(axes.end(), self_normals.begin(), self_normals.end());
    axes.insert(axes.end(), other_normals.begin(), other_normals.end());

    // for every axis
    for (auto& axis : axes)
    {
        // calculate the min and max projection onto an axis for each object
        float minA = INFINITY, maxA = -INFINITY;
        float minB = INFINITY, maxB = -INFINITY;

        for (auto& vertex : self_corners)
        {
            float proj = glm::dot(vertex, axis);
            minA = std::min(minA, proj);
            maxA = std::max(maxA, proj);
        }

        for (auto& vertex : other_corners)
        {
            float proj = glm::dot(vertex, axis);
            minB = std::min(minB, proj);  
            maxB = std::max(maxB, proj);  
        }

        // if an axis is found no collision
        if (maxA < minB || maxB < minA) {
            return false;  
        }
    }

    // no valid axis so collision
    return true;  
}

// helper method to get min/max
// used by valid collision and update
std::pair<float, float> Entity::get_min_max_x() 
{
    std::vector<glm::vec2> corners = this->get_corners();
    float mini = INFINITY, maxi = -INFINITY;
    for (auto& vertex : corners) {
        mini = glm::min(mini, vertex.x);
        maxi = glm::max(maxi, vertex.x);
    }
    std::pair<float, float> val = std::make_pair(mini, maxi);
    return val;
}

std::pair<float, float> Entity::get_min_max_y()
{
    std::vector<glm::vec2> corners = this->get_corners();
    float mini = INFINITY, maxi = -INFINITY;
    for (auto& vertex : corners) {
        mini = glm::min(mini, vertex.y);
        maxi = glm::max(maxi, vertex.y);
    }
    std::pair<float, float> val = std::make_pair(mini, maxi);
    return val;
}

void Entity::valid_collision(Entity* other) {
    // we want collison on top of the platform and since SAT checks for all the other collisions
    // we need to check that the x coordinates are in the range of the start and end of the platform
    // y coordinate should ideally be above the maximum y of the platform 
    // but since im not resetting the y-coor to account for clippin(g it may end up lower

    std::pair<float, float> this_pair = this->get_min_max_x();
    std::pair<float, float> other_pair = other->get_min_max_x();

    float minThis = this_pair.first, maxThis = this_pair.second;
    float minOther = other_pair.first, maxOther = other_pair.second;

    if (!(minThis >= minOther && maxThis <= maxOther)) {
        this->set_status(CRASHED);
        return;
    }

    // check that the angle at landing is within a tolerance of 90 degrees
    if (abs(int(m_angle) % 360 - 90) > 10) 
    { 
        this->set_status(CRASHED);
        return;
    }

    // check that velocity is not to extreme so we're not destroying our thrusters
    // chosen value is less than fabs(0.7), roughly cos(theta/4) or sin(theta/4)
    if (fabs(m_velocity.x) >= 0.7f || fabs(m_velocity.y) >= 0.7f)
    {
        this->set_status(CRASHED);
        return;
    }

    // pass all cases so successful collision
    this->set_status(LANDED);
}

// ----- DEBUG LOG ----- //


const void Entity::log_attributes() {
    std::cout << "Velocity: " << m_velocity.x << " " << m_velocity.y << std::endl;
    std::cout << "Acceleration: " << m_acceleration.x << " " << m_acceleration.y << std::endl;
    std::cout << "Position: " << m_position.x << " " << m_position.y << std::endl;
    std::cout << "Angle: " << m_angle << std::endl;
    std::cout << std::endl;
}


const void Entity::log_corners() {
    std::vector<glm::vec2> corners = get_corners();
    for (size_t i = 0; i < corners.size(); i++) {
        std::cout << "Corner: " << i << " x: " << corners[i].x << " y: " << corners[i].y << std::endl;
    }
    std::cout << std::endl;
}