#include "factory.h"

Factory::Factory(ComponentRegistry& componentRegistry, uint32_t& shader):
componentRegistry(componentRegistry),
shader(shader) {}

unsigned int Factory::allocate_id() {
    
	if (garbage_bin.size() > 0) {
		uint32_t id = garbage_bin[garbage_bin.size() - 1];
		garbage_bin.pop_back();
		return id;
	}
	else {
		uint32_t id = entities_made++;
		return id;
	}
}

void Factory::make_object() {

    unsigned int id = allocate_id();

    // Transform
    TransformComponent transform;

    // Position the object in front of you and a bit higher
    transform.position = glm::vec3(10.0f, 1.5f, -1.5f); // 10 units ahead and 1.5 units above

    // Random rotation
    transform.eulers = glm::vec3(
        360.0f * random_float(), // Random x-axis rotation
        360.0f * random_float(), // Random y-axis rotation
        360.0f * random_float()  // Random z-axis rotation
    );

    // Default scale
    transform.scale = 1.0f;

    componentRegistry.transforms.insert(id, transform);

    // Velocity
    VelocityComponent velocity;

    // Random angular velocity for rotation
    constexpr float maxAngularSpeed = 10.0f;
    velocity.angular = glm::vec3(
        maxAngularSpeed * (2.0f * random_float() - 1.0f), // Random x-axis rotation
        maxAngularSpeed * (2.0f * random_float() - 1.0f), // Random y-axis rotation
        maxAngularSpeed * (2.0f * random_float() - 1.0f)  // Random z-axis rotation
    );

    componentRegistry.velocities.insert(id, velocity);

    // Model
    RenderComponent model;
    model.objectType = static_cast<ObjectType>(random_int_in_range(objectTypeCount)); // Random model type

    componentRegistry.renderables.insert(id, model);
}