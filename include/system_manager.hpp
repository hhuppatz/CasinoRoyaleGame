#include <unordered_map>
#include <memory>
#include <cassert>
#include "systems/game_system.hpp"
#include "component.hpp"

#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

class system_manager
{
public:
	template<typename T> std::shared_ptr<T> register_system();
	template<typename T> void set_signature(signature signature);
	void entity_destroyed(entity entity);
	void entity_signature_changed(entity entity, signature entity_signature);
private:
	// Map from system type string pointer to a signature
	std::unordered_map<const char*, signature> signatures{};
	// Map from system type string pointer to a system pointer
	std::unordered_map<const char*, std::shared_ptr<game_system>> systems{};
};

// Template function definitions
template<typename T> 
std::shared_ptr<T> system_manager::register_system()
{
    const char* typeName = typeid(T).name();

    assert(systems.find(typeName) == systems.end() && "Registering system more than once.");

    // Create a pointer to the system and return it so it can be used externally
    auto system = std::make_shared<T>();
    systems.insert({typeName, system});
    return system;
}

template<typename T> 
void system_manager::set_signature(signature signature)
{
    const char* typeName = typeid(T).name();

    assert(systems.find(typeName) != systems.end() && "System used before registered.");

    // Set the signature for this system
    signatures.insert({typeName, signature});
}

#endif