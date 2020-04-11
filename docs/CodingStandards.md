# Coding Standards for my D3D12 engine

- Use references whenever possible. References represent a non-nullable, non-owning reference to an object
- Use raw pointers for a non-owning, nullable reference to ab object
- Use a smart pointer, like ComPtr or rx::ptr, for a owning reference to an object
- Objects should never store references, only pointers (either raw or smart) and regular values
