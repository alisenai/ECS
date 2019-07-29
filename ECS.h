/*
                                  /    \
                          )      ((   ))     (
 _                       /|\      ))_((     /|\                        _
(@)                     / | \    (/\|/\)   / | \                      (@)
| | ----------------------(  )---\\''//--(  )-------------------------|-|
|-|                        vvv   (o o)   vvv                          | |
| |                              `\Y/'                                |-|
| |                         _____ _____ _____                         | |
|-|                        |   __|     |   __|                        |-|
| |                        |   __|   --|__   |                        | |
|-|                        |_____|_____|_____|                        |-|
| |                                                                   |-|
|_|___________________________________________________________________| |
(@)/             |   /\ /         ( (       \ /\   |                 \(@)
                 | /   V           \ \       V   \ |
                 |/                 ) )           \|
                                    \ /
                                     V
By: Calin Gavriliuc
*/

#ifndef ECS_H
#define ECS_H

#include <unordered_map> //! std::unordered_map
#include <typeindex>     //! std::type_index type
#include <iostream>      //! std:cerr stream

//! Method the component system manager will use to index into it's data storage
#define TYPE_ID_RES typeid(ComponentType)
//! Type for entity IDs
#define ID_TYPE uint64_t
//! Error that occurs when a system of a given component type is not found
#define SYSTEM_TYPE_NOT_FOUND "System of given component type not found"
//! More verbose assert macros
#define M_ASSERT(condition, message) do { if (! (condition)) { std::cerr << "Assertion `" #condition "` failed in " << __FILE__ << " line " << __LINE__ << ": " << message << std::endl; std::terminate(); } } while (false)
//! Uses pack expansion for compile-time variadic template expansion
#define PACK_EXPAND(function, args, a...) (int[]) {0, (function<args>(a), 0)...}

//! Entity class, to be derived from when creating any entities
class Entity
{
public:
    /*!
     * \brief Non-explicit constructor for an entity, given an ID
     * \param id ID of the entity
     */
    Entity(ID_TYPE id) : _id(id)
    {}

    /*!
     * \brief Returns the ID of the entity
     * \return ID of the entity
     */
    ID_TYPE GetId()
    { return _id; }

    /*!
     * \brief Non-explicit conversion operator for an entity
     * \return ID of the entity
     */
    operator ID_TYPE()
    { return _id; }

    /*!
     * \brief Virtual, defaulted, destructor
     */
    virtual ~Entity() = default;

private:
    //! Entity ID
    ID_TYPE _id;
};

//! Pure virtual base component class, to be derived from when creating a component
class Component
{
public:
    /*!
     * \brief Constructor for a component
     * \param parentID ID of the parent Entity
     * \attention If Parent ID is -1, something might have gone wrong (default parent ID)
     */
    Component()
    { _parentID = -1; }

    /*!
     * \brief Returns the parent ID of the component
     * \return Parent ID of the component
     */
    ID_TYPE GetParentID()
    { return _parentID; }

    /*!
     * \brief Sets the component's parent ID
     * \param parentID Component's parent ID
     */
    void SetParentID(ID_TYPE parentID)
    { _parentID = parentID; }

    /*!
     * \brief Pure virtual update function. Required to be implemented in derived classes
     * \param dt Delta time / time since the last tick, to be used when updating
     */
    virtual void Update(float dt) = 0;

    /*!
     * \brief Virtual, defaulted, destructor
     */
    virtual ~Component() = default;

private:
    //! Parent entity ID
    ID_TYPE _parentID;
};

/*!
 * \brief Base component system interface for all component systems
 * \tparam ComponentType Component type to handle
 */
template<class ComponentType>
class ComponentSystemInterface
{
public:
    /*!
     * \brief Cleans up all the trailing components
     */
    ~ComponentSystemInterface()
    {
        // Free every component
        for (auto pair : _components)
            delete pair.second;
    }

    /*!
     * \brief Adds a component to the system, given an ID to associate with. If the passed component is NULL, it will
     * delete the component currently attached
     * \param id Entity ID to associate with
     * \param component Component to add
     */
    void AddComponent(ID_TYPE id, ComponentType *component)
    {
        // If there's already a component at that ID, delete it
        RemoveComponent(id);
        // Add the component to the requested ID
        if (component)
        {
            // Set the parent ID
            component->SetParentID(id);
            _components[id] = component;
        }
    }

    /*!
     * \brief Returns the component associated with the given ID
     * \param id ID associated with the component
     * \return Component of given type with associated ID, NULL if it doesn't exist
     */
    ComponentType *GetComponent(ID_TYPE id)
    {
        // Index into components, returning the item at the given ID
        return _components[id];
    }

    /*!
     * \brief Removes a component associated with the given ID
     * \param id ID associated with the component
     */
    void RemoveComponent(ID_TYPE id)
    {
        // If the component exists
        if (_components[id])
        {
            // Clean up the memory
            delete _components[id];
            // Remove the component at the given ID
            _components.erase(id);
        }
    }

    /*!
     * \brief Updates every component in the system, passing dt
     * \param dt Delta time / time since last tick to pass
     */
    void UpdateSystem(float dt)
    {
        // Update each component in the system
        for (auto it = _components.begin(); it != _components.end(); ++it)
            it->second->Update(dt);
    }

    /*!
     * \brief Get a reference to the components within the system
     * \return Return a reference to all the components within the system
     */
    auto &GetComponents()
    {
        // Return a reference to the components this system manages
        return _components;
    }

private:
    //! Key:ID, Value:Component Pointer
    std::unordered_map<ID_TYPE, ComponentType *> _components;
};

/*!
 * \brief Default component system for any component type. Specialize for custom behavior
 * \tparam ComponentType Component type to handle
 */
template<class ComponentType>
class ComponentSystem : public ComponentSystemInterface<ComponentType>
{
};

/*!
 * \brief Manager class for systems of variadic component types
 * \tparam ComponentTypes Variadic list of component types to handle
 */
template<class ...ComponentTypes>
class ComponentSystemManager
{
public:
    /*!
     * \brief Register component systems of every given type
     */
    ComponentSystemManager()
    {
        // Make sure each type given is valid
        PACK_EXPAND(CheckValidType, ComponentTypes);
        // Pack expands each variadic template class parameter
        PACK_EXPAND(RegisterComponentSystem, ComponentTypes);
    }

    /*!
     * \brief Unregister component systems of every given type
     */
    ~ComponentSystemManager()
    {
        // Pack expands each variadic template class parameter
        PACK_EXPAND(UnregisterComponentSystem, ComponentTypes);
    }

    /*!
     * \brief Returns a component of the given type associated with the given ID
     * \tparam ComponentType Component type to get (and return!)
     * \param id ID associated with the component
     * \return Component of given type T associated with the given ID
     */
    template<class ComponentType>
    ComponentType *GetComponent(ID_TYPE id)
    {
        // Grab the component system pointer
        void *componentSystemPointer = componentSystems[TYPE_ID_RES];
        // Make sure the system exists
        M_ASSERT(componentSystemPointer != nullptr, SYSTEM_TYPE_NOT_FOUND);
        // Return the component from the system
        return static_cast<ComponentSystem<ComponentType> *>(componentSystemPointer)->GetComponent(id);
    }

    /*!
     * \brief Adds a given component of the given type with the given ID
     * \tparam ComponentType Type of the component to add
     * \param id ID to associate with the component
     * \param component Component to add
     */
    template<class ComponentType>
    void AddComponent(ID_TYPE id, ComponentType *component) noexcept(false)
    {
        // Grab the component system pointer
        void *componentSystemPointer = componentSystems[TYPE_ID_RES];
        // Make sure the system exists
        M_ASSERT(componentSystemPointer != nullptr, SYSTEM_TYPE_NOT_FOUND);
        // Add the component to the system
        static_cast<ComponentSystem<ComponentType> *>(componentSystemPointer)->AddComponent(id, component);
    }

    /*!
     * \brief Removes a component of the given type associated with the given ID
     * \tparam ComponentType Type of the component to remove
     * \param id ID associated with the component
     */
    template<class ComponentType>
    void RemoveComponent(ID_TYPE id) noexcept(false)
    {
        // Grab the component system pointer
        void *componentSystemPointer = componentSystems[TYPE_ID_RES];
        // Make sure the system exists
        M_ASSERT(componentSystemPointer != nullptr, SYSTEM_TYPE_NOT_FOUND);
        // Try to remove a component from the system
        static_cast<ComponentSystem<ComponentType> *>(componentSystemPointer)->RemoveComponent(id);
    }

    /*!
     * \brief Updates a system of the given component type, passing dt
     * \tparam ComponentType Component type to update the system of
     * \param dt Delta time / time since the last tick to update the system with
     */
    template<class ComponentType>
    void UpdateSystem(float dt) noexcept(false)
    {
        // Grab the component system pointer
        void *componentSystemPointer = componentSystems[TYPE_ID_RES];
        // Make sure the system exists
        M_ASSERT(componentSystemPointer != nullptr, SYSTEM_TYPE_NOT_FOUND);
        // Update the system
        static_cast<ComponentSystem<ComponentType> *>(componentSystemPointer)->UpdateSystem(dt);
    }

    /*!
     * \brief Updates the systems of every component type, passing dt
     * \param dt Delta time / time since the last tick to update the systems with
     */
    void UpdateSystems(float dt) noexcept(false)
    {
        // Calls templated function UpdateSystem
        PACK_EXPAND(UpdateSystem, ComponentTypes, dt);
    }

    /*!
     * \brief Returns a pointer the the system of the given type
     * \tparam ComponentType Type of the system to return
     * \return Pointer to the system of the given type, or NULL if it doesn't exist
     */
    template<class ComponentType>
    ComponentSystem<ComponentType> *GetComponentSystem()
    {
        return static_cast<ComponentSystem<ComponentType> *>(componentSystems[TYPE_ID_RES]);
    }

private:
    /*!
     * \brief Checks if the given type is valid to be handled by this class
     * \tparam ComponentType Type to check the validity of
     */
    template<class ComponentType>
    void CheckValidType()
    {
        // Check that the component type given has a base of Component
        M_ASSERT((std::is_base_of<Component, ComponentType>::value), "Passed class type should inherit from Component");
    }

    /*!
     * \brief Registers a component system of the given type
     * \tparam ComponentType Type of the system to register
     */
    template<class ComponentType>
    void RegisterComponentSystem()
    {
        // Register a new component system
        componentSystems[TYPE_ID_RES] = new ComponentSystem<ComponentType>;
    }

    /*!
     * \brief Unregisters a component system of the given type
     * \tparam ComponentType Type of the system to unregister
     */
    template<class ComponentType>
    void UnregisterComponentSystem()
    {
        // Free system of given type from the map
        delete static_cast<ComponentSystem<ComponentType> *>(componentSystems[TYPE_ID_RES]);
        // Delete it from the component systems
        componentSystems.erase(TYPE_ID_RES);
    }

    //! Component systems handled by this manager
    std::unordered_map<std::type_index, void *> componentSystems;
};

#endif //ECS_H
