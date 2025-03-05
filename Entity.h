#ifndef ENTITY_H
#define ENTITY_H

#include "glm/glm.hpp"
#include "ShaderProgram.h"

enum AngleDirection { LEFT, RIGHT, NONE };
enum FuelDirection { UP, DOWN };


class Entity
{
private:

	// ----- TRANSFORMATIONS ----- //
	glm::vec3 m_position;
	glm::vec3 m_movement;
	glm::vec3 m_scale;
	glm::vec3 m_velocity;
	glm::vec3 m_acceleration;
	glm::vec3 m_rotation; // rotation matrix, about which axis

	glm::mat4 m_model_matrix;

	float m_speed;
	float m_angle; // angle accumulator

	// ----- TEXTURES ----- //
	GLuint m_texture_id;

	// ----- ANIMATIONS ----- //


	// ----- COLLISIONS ----- //


public:
	// ----- STATIC VARIABLES ----- //
	static constexpr int SECONDS_PER_FRAME = 1;
	static constexpr float ANGLE_PER_TIME = 15.0f;

	// ----- METHODS ----- //
	Entity();
	Entity(GLuint texture_id, float speed, glm::vec3 acceleration);
	~Entity();

	void update(float delta_time);
	void render(ShaderProgram* program);
	void rotate(float delta_time, AngleDirection dir);
	void updateFuel(float delta_time, FuelDirection direction);

	// ----- GETTERS ----- //
	glm::vec3 const get_position()     const { return m_position; }
	glm::vec3 const get_velocity()     const { return m_velocity; }
	glm::vec3 const get_acceleration() const { return m_acceleration; }
	glm::vec3 const get_movement()     const { return m_movement; }
	glm::vec3 const get_scale()        const { return m_scale; }
	GLuint    const get_texture_id()   const { return m_texture_id; }

	// ----- SETTERS ----- //
	void const set_position(glm::vec3 new_position) { m_position = new_position; }
	void const set_velocity(glm::vec3 new_velocity) { m_velocity = new_velocity; }
	void const set_acceleration(glm::vec3 new_acceleration) { m_acceleration = new_acceleration; }
	void const set_movement(glm::vec3 new_movement) { m_movement = new_movement; }
	void const set_scale(glm::vec3 new_scale) { m_scale = new_scale; }
	void const set_texture_id(GLuint new_texture_id) { m_texture_id = new_texture_id; }
};

#endif // ENTITY_H