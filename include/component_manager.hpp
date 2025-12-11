#include <memory>
#include <unordered_map>
#include <cassert>
#include <typeinfo>
#include "component.hpp"
#include "i_component_array.hpp"

#ifndef COMPONENT_MANAGER_H
#define COMPONENT_MANAGER_H

class component_manager
{
public:
	template<typename T> void register_component();
	template<typename T> component_type get_component_type();
	template<typename T> void add_component(entity entity, T component);
	template<typename T> void remove_component(entity entity);
	template<typename T> T& get_component(entity entity);
	template<typename T> bool has_component(entity entity);
	void entity_destroyed(entity entity);
private:
	// Map from type string pointer to a component type
	std::unordered_map<const char*, component_type> component_types{};

	// Map from type string pointer to a component array
	std::unordered_map<const char*, std::shared_ptr<i_component_array>> component_arrays{};

	// The component type to be assigned to the next registered component - starting at 0
	component_type next_component_type{};

	// Convenience function to get the statically casted pointer to the ComponentArray of type T.
	template<typename T>
	std::shared_ptr<component_array<T>> get_component_array();
};

// Template function definitions
template<typename T>
void component_manager::register_component()
{
    const char* typeName = typeid(T).name();

    assert(component_types.find(typeName) == component_types.end() && "Registering component type more than once.");

    // Add this component type to the component type map
    component_types.insert({typeName, next_component_type});

    // Create a ComponentArray pointer and add it to the component arrays map
    component_arrays.insert({typeName, std::make_shared<component_array<T>>()});

    // Increment the value so that the next component registered will be different
    ++next_component_type;
}

template<typename T>
component_type component_manager::get_component_type()
{
    const char* typeName = typeid(T).name();

    assert(component_types.find(typeName) != component_types.end() && "Component not registered before use.");

    // Return this component's type - used for creating signatures
    return component_types[typeName];
}

template<typename T>
void component_manager::add_component(entity entity, T component)
{
    // Add a component to the array for an entity
    get_component_array<T>()->insert_data(entity, component);
}

template<typename T>
void component_manager::remove_component(entity entity)
{
    // Remove a component from the array for an entity
    get_component_array<T>()->remove_data(entity);
}

template<typename T>
T& component_manager::get_component(entity entity)
{
    // Get a reference to a component from the array for an entity
    return get_component_array<T>()->get_data(entity);
}

template<typename T>
bool component_manager::has_component(entity entity)
{
    // Check if an entity has a component
    return get_component_array<T>()->has_data(entity);
}

// Convenience function to get the statically casted pointer to the ComponentArray of type T.
template<typename T>
std::shared_ptr<component_array<T>> component_manager::get_component_array()
{
    const char* type_name = typeid(T).name();

    assert(component_types.find(type_name) != component_types.end() && "Component not registered before use.");

    return std::static_pointer_cast<component_array<T>>(component_arrays[type_name]);
}

#endif