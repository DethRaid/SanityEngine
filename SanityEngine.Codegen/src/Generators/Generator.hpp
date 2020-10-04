#pragma once

namespace cppast {
	// ReSharper disable once CppInconsistentNaming
	class cpp_class;
}

namespace Sanity::Codegen {
    class Generator
    {
    public:
        virtual ~Generator() = default;

    	virtual void GenerateForClass(const cppast::cpp_class& entity) = 0;
    };
} // namespace Sanity::Codegen
