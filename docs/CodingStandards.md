# Coding Standards for my engine

- Use references whenever possible. References represent a non-nullable, non-owning reference to an object
- Use raw pointers for a non-owning, nullable reference to ab object
- Use a smart pointer, like ComPtr or std::unique_ptr, for a owning reference to an object
- Objects should never store references, only pointers (either raw or smart) and regular values
