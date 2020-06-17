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
