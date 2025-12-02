#ifndef ECS_H
#define ECS_H

#include <unordered_map>
#include <assert.h>
#include <cstdint>
#include <memory>
#include <bitset>
#include <stack>
#include <set>

/////////////////////////////// COMPONENTS DECLARATION ////////////////////////////////

using ComponentID = std::uint8_t; // Component ids
const std::uint8_t MAX_COMPONENTS = 255;

template<typename...>
struct ComponentList;

//////////////////////////////// ENTITIES DECLARATION ////////////////////////////////

using EntityID = std::uint32_t; // Entity Ids
using Signature = std::bitset<MAX_COMPONENTS>;
const std::uint32_t MAX_ENTITIES = 5000;


///////////////////////////////// SYSTEMS DECLARATION /////////////////////////////////

struct System {
	std::set<EntityID> entities;
    virtual void run() = 0;
};

/////////////////////////////////// ECS VARIABLES /////////////////////////////////////

std::unordered_map<const char*, void*> ecs_resources;
// store each system required components
std::unordered_map<const char*, Signature> ecs_system_signatures{}; 
std::unordered_map<const char*, std::shared_ptr<System>> ecs_systems{};

std::stack<EntityID> ecs_available_entities_stack{};
Signature ecs_entity_signatures[MAX_ENTITIES];

////////////////////////////////////// ECS SETUP //////////////////////////////////////

void ecs_setup() {
    for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity) ecs_available_entities_stack.push(entity);
}

void ecs_cleanup() {
    for (auto const& pair : ecs_resources) {
        auto const& resource = pair.second;
        free(resource);
    }
}

//////////////////////////////////// DECLARATIONS ////////////////////////////////////

void ecs_entity_set_signature(EntityID entity, Signature signature);
Signature ecs_entity_get_signature(EntityID entity);

/////////////////////////////////// COMPONENTS IMPL ///////////////////////////////////

class IComponentArray {
public:
	virtual ~IComponentArray() = default;
	virtual void entity_destroyed(EntityID entity) = 0;
};

template<typename COMPONENTTYPE>
struct ComponentArray : IComponentArray {
    // The packed array of components (of generic type T),
	// set to a specified maximum amount, matching the maximum number
	// of entities allowed to exist simultaneously, so that each entity
	// has a unique spot.
    // std::array<COMPONENTTYPE,MAX_ENTITIES> mComponentArray;
    COMPONENTTYPE mComponentArray[MAX_ENTITIES];
    // Map from an entity ID to an array index.
	std::unordered_map<EntityID, size_t> mEntityToIndexMap;
	// Map from an array index to an entity ID.
	std::unordered_map<size_t, EntityID> mIndexToEntityMap;
    // Total size of valid entries in the array.
	size_t mSize;

    void insert_data(EntityID entity, COMPONENTTYPE component) {
        assert(mEntityToIndexMap.find(entity) == mEntityToIndexMap.end() && "Component added to same entity more than once.");

		// Put new entry at end and update the maps
		size_t newIndex = mSize;
		mEntityToIndexMap[entity] = newIndex;
		mIndexToEntityMap[newIndex] = entity;
		mComponentArray[newIndex] = component;
		++mSize;
    }

    void remove_data(EntityID entity) {
        assert(mEntityToIndexMap.find(entity) != mEntityToIndexMap.end() && "Removing non-existent component.");

		// Copy element at end into deleted element's place to maintain density
		size_t indexOfRemovedEntity = mEntityToIndexMap[entity];
		size_t indexOfLastElement = mSize - 1;
		mComponentArray[indexOfRemovedEntity] = mComponentArray[indexOfLastElement];

		// Update map to point to moved spot
		EntityID entityOfLastElement = mIndexToEntityMap[indexOfLastElement];
		mEntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
		mIndexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

		mEntityToIndexMap.erase(entity);
		mIndexToEntityMap.erase(indexOfLastElement);

		--mSize;
    }

    COMPONENTTYPE& get_data(EntityID entity) {
        assert(mEntityToIndexMap.find(entity) != mEntityToIndexMap.end() && "Retrieving non-existent component.");

		// Return a reference to the entity's component
		return mComponentArray[mEntityToIndexMap[entity]];
        
    }

    void entity_destroyed(EntityID entity) override {
		if (mEntityToIndexMap.find(entity) != mEntityToIndexMap.end()) {
			// Remove the entity's component if it existed
			remove_data(entity);
		}
	}
};

std::unordered_map<const char*, ComponentID> ecs_component_types{};
std::unordered_map<const char*, std::shared_ptr<IComponentArray>> ecs_component_array{};
std::uint8_t ecs_component_count = 0;

template<typename COMPONENTTYPE>
std::shared_ptr<ComponentArray<COMPONENTTYPE>> ecs_get_component_array() {
    const char* typeName = typeid(COMPONENTTYPE).name();

    assert(ecs_component_types.find(typeName) != ecs_component_types.end() && "Component not registered before use.");

    return std::static_pointer_cast<ComponentArray<COMPONENTTYPE>>(ecs_component_array[typeName]);
}

/// @brief get the array containing all component of the same type
template<typename COMPONENTTYPE>
void ecs_register_component() {
    const char* typeName = typeid(COMPONENTTYPE).name();

    assert(ecs_component_types.find(typeName) == ecs_component_types.end() && "Registering component type more than once.");

    ecs_component_types.insert({typeName, ecs_component_count});
    ecs_component_array.insert({typeName, std::make_shared<ComponentArray<COMPONENTTYPE>>()});

    ++ecs_component_count;
}

template<typename COMPONENTTYPE>
ComponentID ecs_get_component_type() {
    const char* typeName = typeid(COMPONENTTYPE).name();
    
    assert(ecs_component_types.find(typeName) != ecs_component_types.end() && "Component not registered before use.");
    
    return ecs_component_types[typeName];
}

template<typename COMPONENTTYPE>
COMPONENTTYPE& ecs_add_component(EntityID entity) {
    ecs_get_component_array<COMPONENTTYPE>()->insert_data(entity,COMPONENTTYPE());
    auto signature = ecs_entity_get_signature(entity);
    signature.set(ecs_get_component_type<COMPONENTTYPE>(),true);
    ecs_entity_set_signature(entity,signature);
    return ecs_get_component_array<COMPONENTTYPE>()->get_data(entity);
}

template<typename COMPONENTTYPE>
void ecs_remove_component(EntityID entity) {
    ecs_get_component_array<COMPONENTTYPE>()->remove_data(entity);

    auto signature = ecs_entity_get_signature(entity);
    signature.set(ecs_get_component_type<COMPONENTTYPE>(),false);
    ecs_entity_set_signature(entity,signature);
}

template<typename COMPONENTTYPE>
COMPONENTTYPE* ecs_get_component(EntityID entity) {
    return &ecs_get_component_array<COMPONENTTYPE>()->get_data(entity);
}

/////////////////////////////////// ENTITIES IMPL ///////////////////////////////////

EntityID ecs_add_entity() {
    EntityID id = ecs_available_entities_stack.top();
    ecs_available_entities_stack.pop();
    return id;
}

void ecs_entity_destroyed(EntityID entity) {
    for (auto const& pair : ecs_component_array) {
        auto const& component = pair.second;
        component->entity_destroyed(entity);
    }
}

inline void ecs_remove_entity(EntityID entity) {
    ecs_available_entities_stack.push(entity);
    ecs_entity_signatures[entity].reset();

    // Notify each systems that an entity has been destroyed
    for (auto const& pair : ecs_systems) {
        auto const& system = pair.second;

        system->entities.erase(entity);
    }

    // Notify each component array that an entity has been destroyed
    for (auto const& pair : ecs_component_array) {

        auto const& component = pair.second;

        component->entity_destroyed(entity);
    }
}

void ecs_entity_set_signature(EntityID entity,Signature entitySignature) {
    ecs_entity_signatures[entity] = entitySignature;

    // Notify each system that an entity's signature changed
    for (auto const& pair : ecs_systems) {

        auto const& systemtype = pair.first;
        auto const& system = pair.second;
        auto const& systemSignature = ecs_system_signatures[systemtype];

        if ((entitySignature & systemSignature) == systemSignature) {
            // Entity signature matches system signature - insert into set
            system->entities.insert(entity);
        } else {
            // Entity signature does not match system signature - erase from set
            system->entities.erase(entity);
        }
    }
}

Signature ecs_entity_get_signature(EntityID entity) {
	return ecs_entity_signatures[entity];
}


/////////////////////////////////// SYSTEMS IMPL ///////////////////////////////////

template<typename SYSTEM>
void ecs_add_system() {
    const char* typeName = typeid(SYSTEM).name();

    assert(ecs_systems.find(typeName) == ecs_systems.end() && "adding system more than once.");

    ecs_system_signatures[typeName].reset();
    auto system = std::make_shared<SYSTEM>();
    ecs_systems.insert({typeName, system});
}

template<typename SYSTEM,typename COMPONENT0,typename... COMPONENT1TON>
void ecs_add_system() {
    const char* typeName = typeid(SYSTEM).name();

    assert(ecs_systems.find(typeName) == ecs_systems.end() && "adding system more than once.");

    
    ecs_system_signatures[typeName].set(ecs_get_component_type<COMPONENT0>());
    (ecs_system_signatures[typeName].set(ecs_get_component_type<COMPONENT1TON>()), ...);
    
    auto system = std::make_shared<SYSTEM>();
    ecs_systems.insert({typeName, system});
}

void ecs_run_systems() {
    for (auto const& pair : ecs_systems) {
        auto const& system = pair.second;
        system->run();
    }
}

////////////////////////////////// RESOURCES IMPL //////////////////////////////////

template<typename T>
void ecs_add_resource() {
    const char* typeName = typeid(T).name();
    
    assert(ecs_resources.find(typeName) == ecs_resources.end() && "Registering resource more than once.");
    
    ecs_resources.insert({typeName, malloc(sizeof(T))});
}

template<typename T>
void ecs_remove_resource() {
    const char* typeName = typeid(T).name();
    
    assert(ecs_resources.find(typeName) != ecs_resources.end() && "Removing resource that is not registered.");
    
    free(ecs_resources[typeName]);
    ecs_resources.erase(typeName);
}

template<typename T>
T* ecs_get_resource() {
    const char* typeName = typeid(T).name();

    assert(ecs_resources.find(typeName) != ecs_resources.end() && "Getting resource that is not registered.");

    return (T*)ecs_resources[typeName];
}

#endif