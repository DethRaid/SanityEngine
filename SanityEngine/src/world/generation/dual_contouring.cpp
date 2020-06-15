#include "dual_contouring.hpp"

// ReSharper disable once CppInconsistentNaming
namespace _detail {
    float adapt(const Int32& v0, const Int32& v1) {
        RX_ASSERT((v0 > 0) != (v1 > 0), "Numbers do not have opposide sign");

        return static_cast<float>(0 - v0) / static_cast<float>(v1 - v0);
    }

    Vec3f solve_qef(Uint32 x, Uint32 y, Uint32 z, const Rx::Vector<Vec3f>& vertices, const Rx::Vector<Vec3f>& normals) {
        return {};
    }
} // namespace _detail
