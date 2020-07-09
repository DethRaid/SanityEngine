class Entity {
    foreign get_tags()

    foreign get_world()
}

/*!
 * \brief A component
 *
 * Components work in a very specific way:
 *
 * SanityEngine creates an instance of every Component when it starts up, and  saves that instance to RAM. Later on,
 * when someone creates a Component, SanityEngine creates a copy of the default instance of that Componenet, then 
 * copies a data table of values into the new Component, then calles `begin_play()` on the new Component. The next 
 * frame, SanityEngine begins calling `tick()` on the new Component. It will call `tick()` every frame until the 
 * Component is destroyed. The frame after the Component is destroyed, SanityEngine will call `end_play()` on the 
 * Component, before finally deallocating the Component
 * 
 * This has a number of implications. The constructor of a Component may access all fields within the class, along 
 * with any foreign code except for `get_entity()`. 
 */
class Component {
    /*!
     * \brief Initializes the component in a context without an active game world
     *
     * Constructors should only set up the default instance of a component. This might involve calculating default
     * values for fields or loading resources from disk. Any of a component's fields may be changed when the engine 
     * loads the data table for an instance of this component
     *
     * Should NOT try to reference the entity that this component is attached to, other components on the entity, etc
     * Should ONLY touch values in this component
     */
    construct new() {}

    /*!
     * \brief Initializes this component and its interactions with other objects in a world, including its entity or 
     * other components
     *
     * This is called after the component has
     */
    begin_play(world) {}

    /*!
     * \brief Initializes this component's internal simulation, advancing internal state by the provided time step
     */
    tick(delta_time) {}

    /*!
     * \brief Does everything necessary to prepare for destroying this component
     *
     * This should do things like unregister the component from various systems
     */
    end_play() {}

    /*!
     * \brief Gets the entity that this component is attached to
      */
    foreign get_entity()
}

class TestComponent is Component {
    tick(delta_time) {
        System.print("Ticking...")
    }
}
