foreign class World {}

foreign class SanityEngine {}

foreign class Entity {
    foreign get_tags()

    foreign World get_world()
}

foreign class Component {
    /*!
     * \brief Initializes the component in a context without an active game world
     *
     * Should NOT try to reference the entity that this component is attached to, other components on the entity, etc
     * Should ONLY touch values in this component
     */
    initialize_self() {}

    /*!
     * \brief Initializes this component and its interactions with other objects in a world, including its entity or other components
     */
    begin_play(World world) {}

    /*!
     * \brief Initializes this component's internal simulation, advancing internal state by the provided time step
     */
    tick(Num delta_time) {}

    /*!
     * \brief Does everything necessary to prepare for destroying this component
     *
     * This should do things like unregister the component from various systems
     */
    end_play() {}
}

class TestComponent is Component {
    tick(Num delta_time) {
        System.print("Ticking...")
    }
}
