# AI description

## Dynamic

The AI will be very dynamic. It must be able to respond to a vast number of unique worlds, each with their own challenges. Here's some stuff I'll use to achieve that:

### Pathing

Pathing will largely rely on A*. We'll have one node for every exposed face of a block. These nodes wil be connected to each other in very specific ways. Different blocks will have different path costs, such that AIs prefer paths over wading through a stream, for example. The block path costs will be partly dependent on the environment, using the procedurally generated biome system from the terrain

### Needs

AIs model a modified version of Maslow's hierarchy of needs

Or maybe they have something based on the needs of their body and their brain chemicals?

Either way, each AI has a ordered list of needs that they much constantly fulfil

AIs know about a number of ways to fulfil each need, and how strongly that fulfils the need. AIs have different values for what fulfils each need. This enables a diverse society

Objects have a number of tags that determine what role they fulfil in that hierarchy. Some objects, such as apples, can directly influence a need. Other objects can be used by the AI to create different objects that fulfil the needs differently. For example, an apple may be baked into an apple pie, which would fulfil some of the hunger need. How much an object influences a need is determined partly by the object and partly by the AI. An apple always fulfils the hunger need - that's a physical property of both the apple and the person eating it. However, an apple might also fulfil a person's happiness needs depending on how the person's senses perceive it. One person thinks that apples are delicious, another person thinks that apples just look pretty, etc

When the system needs to fulfil a need, it finds all the objects and actions that the person knows about which can fulfil that need. Eating an apple fulfils hunger, baking an apple pie produces an apple pie which can fulfil more hunger. The person can weigh those against how much it helps the person vs how much is helps the person's friends vs how much it hurts this person-s enemies vs other things I might care about. Based on all those calculations, the person determines what to do

Non-characters will also make use of this system. Animals will probably end up more sense-driven than happiness-driven - they won't be able to comprehend baking an apple pie, so they will only know about apples

## Intelligence

Intelligence will me modeled in a couple ways

### Memory capacity
    
Some AIs will only be able to hold more in their memory than others. This lets them remember the locations of objects for a longer period of time

### Reasoning ability
    
Each actions is made up of a certain number of discreet steps. Some actions are shorter, some are longer. More intelligent AIs will be able to learn and perform actions with more steps

### Knowledge

More intelligent AIs will be able to learn more facts about the world and will be able to learn more actions they can perform

"Facts about the world" is how to combine an object's properties. For instance, an AI might learn that adding water to fire will remove the fire. It might also learn that water removes heat from an object, and it might learn that removing heat from the fire puts out the fire, and from those it might synthesize that adding water to fire puts out fire. More intelligent AIs would both be able to synthesize together longer chains, and would be able to learn more facts about the world

### Curiosity

AIs will 

### Learn by example

AIs will be able to learn from observation. They can observe an interactions between two objects and remember that interaction on whatever level they perceive

For example, an AI might observe that falling rain puts out torches. They'll observe that with their eyes, meaning they just see that lit torch + rain = unlit torch. They may or may not know 
