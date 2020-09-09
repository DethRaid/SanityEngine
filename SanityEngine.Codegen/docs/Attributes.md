SanityEngine.Codegen recognizes a number of attributes in your C++ files

# Class Attributes

## `[sanity::runtimeclass]`

The runtime class attribute marks this class as part of the cross-language SanityEngine API. All public methods will be
exposed in the API as methods, all public variables will be exposed in the API as properties. All the types used in the
runtime API must be either primitive types (including strings and lists) of other runtime classes
