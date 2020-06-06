using System.Runtime.InteropServices;

namespace SanityEngine
{
    [ComVisible(true)]
    [ClassInterface(ClassInterfaceType.AutoDual)]
    public class GameplayComponent
    {
        /// <summary>
        /// Initializes this component when it's first created
        /// </summary>
        /// This method should only read from or write to values on this component. Any kind of work done to hook up
        /// this component to the outside world should be called in BeginPlay
        public virtual void InitializeSelf(/* TODO: Pass in a reference to the game mode */ )
        {
            
        }

        /// <summary>
        /// Initializes this component in a game world
        /// </summary>
        /// This method should do whatever it needs to register this component with the game world. If you need to
        /// register objects with runtime systems, cache references to other components, and otherwise interact with
        /// other objects in the game world, do so here
        public virtual void BeginPlay(/* TODO: Pass in a reference to the game world */)
        {
        }

        /// <summary>
        /// Ticks this component, advancing any internal simulation by the provided amount of time
        /// </summary>
        /// <param name="deltaTime">Seconds since the last time Tick was called</param>
        public virtual void Tick(float deltaTime)
        {
        }

        /// <summary>
        /// Prepares this object for removal from the game world
        /// </summary>
        /// This should clean up any references to the component that were created in BeginPlay, should despawn any
        /// objects that it spawned
        public virtual void EndPlay()
        {
        }
    }
}
