#ifndef ENTITY_H
#define ENTITY_H

#include "glm/glm.hpp"
#include "ShaderProgram.h"

#include <vector>

enum AngleDirection { LEFT, RIGHT, NONE };
enum EntityStatus { CRASHED, LANDED, ACTIVE, START };

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

	float	m_speed;
	float	m_angle; // angle accumulator
	int		m_fuel;
	float	m_width;
	float	m_height;

	bool m_use_acceleration;
	
	
	EntityStatus m_status;

	// ----- TEXTURES ----- //
	GLuint m_texture_id;

	// ----- ANIMATIONS ----- //


	// ----- COLLISIONS ----- //


public:
	// ----- STATIC VARIABLES ----- //
	static constexpr int	SECONDS_PER_FRAME = 1;
	static constexpr float	ANGLE_PER_TIME = 90.0f;
	static constexpr float	GRAVITY = 0.2f;
	static constexpr int	FUEL_PER_TIME = 1;
	static constexpr float	ACCEL_SCALE = 1.0f;

	// ----- METHODS ----- //
	Entity();
	Entity(GLuint texture_id, float speed, glm::vec3 acceleration, bool use_accel, EntityStatus status);
	~Entity();

	// logs
	const void log_attributes();
	const void log_corners();

	void update(float delta_time, Entity* collidable_entities, int collidable_entity_count);
	void render(ShaderProgram* program);
	void rotate(float delta_time, AngleDirection dir);
	void update_fuel(float delta_time, bool using_fuel);

	void set_dimensions(float x, float y);

	// SAT collision cause box collisions are janky
	std::vector<glm::vec2> get_corners();
	std::vector<glm::vec2> get_edges();
	std::vector<glm::vec2> get_normals();
	bool check_collision_SAT(Entity* other);

	void valid_collision(Entity* other);


	// ----- GETTERS ----- //
	glm::vec3		const	get_position()     const { return m_position; }
	glm::vec3		const	get_velocity()     const { return m_velocity; }
	glm::vec3		const	get_acceleration() const { return m_acceleration; }
	glm::vec3		const	get_movement()     const { return m_movement; }
	glm::vec3		const	get_scale()        const { return m_scale; }
	GLuint			const	get_texture_id()   const { return m_texture_id; }
	int				const	get_fuel()		   const { return m_fuel; }
	float			const	get_angle()		   const { return m_angle; }
	EntityStatus	const	get_status()	   const { return m_status; }

	// ----- SETTERS ----- //
	void const set_position(glm::vec3 new_position) { m_position = new_position; }
	void const set_velocity(glm::vec3 new_velocity) { m_velocity = new_velocity; }
	void const set_acceleration(glm::vec3 new_acceleration) { m_acceleration = new_acceleration; }
	void const set_movement(glm::vec3 new_movement) { m_movement = new_movement; }
	void const set_scale(glm::vec3 new_scale) { m_scale = new_scale; }
	void const set_texture_id(GLuint new_texture_id) { m_texture_id = new_texture_id; }
	void const set_status(EntityStatus new_status) { m_status = new_status;  }
};

#endif // ENTITY_H