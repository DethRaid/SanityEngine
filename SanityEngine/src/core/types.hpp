#pragma once

#include <guiddef.h>
#include <stddef.h>

#include "rx/core/types.h"
#include "rx/math/mat4x4.h"
#include "rx/math/vec2.h"
#include "rx/math/vec3.h"
#include "rx/math/vec4.h"

using Rx::Byte;
using Rx::Size;

using Rx::Uint16;
using Rx::Uint32;
using Rx::Uint64;
using Rx::Uint8;

using Int8 = Rx::Sint8;
using Int16 = Rx::Sint16;
using Int32 = Rx::Sint32;
using Int64 = Rx::Sint64;

using Rx::Float32;
using Rx::Float64;

using Rx::Math::Vec2f;
using Rx::Math::Vec3f;
using Rx::Math::Vec4f;

using Rx::Math::Vec2i;
using Rx::Math::Vec3i;
using Rx::Math::Vec4i;

using Uint2 = Rx::Math::Vec2<Uint32>;
using Uint3 = Rx::Math::Vec3<Uint32>;
using Uint4 = Rx::Math::Vec4<Uint32>;

using Double2 = Rx::Math::Vec2<Float64>;
using Double3 = Rx::Math::Vec3<Float64>;
using Double4 = Rx::Math::Vec4<Float64>;

using Rx::Math::Mat4x4f;

using Rx::operator""_z;
using Rx::operator""_u8;
using Rx::operator""_u16;
using Rx::operator""_u32;
using Rx::operator""_u64;

namespace sanity::engine {
    template <class _Ty, class = void>
    struct _Add_reference { // add reference (non-referenceable type)
        using _Lvalue = _Ty;
        using _Rvalue = _Ty;
    };

    template <class _Ty>
    using add_rvalue_reference_t = typename _Add_reference<_Ty>::_Rvalue;

    template <class _From, class _To>
    concept convertible_to = __is_convertible_to(_From, _To) && requires(add_rvalue_reference_t<_From> (&_Fn)()) {
        static_cast<_To>(_Fn());
    };

    template <typename TestType>
    concept ComInterface = requires(TestType test_obj) {
        { test_obj.AddRef() }
        ->convertible_to<Uint32>;
        { test_obj.Release() }
        ->convertible_to<Uint32>;
    };

    /**
     * @brief Smart pointer for COM types, such as all of D3D12
     *
     * I made my own because Microsoft::WRL::ComPtr is in `wrl/client.h`, and `wrl/client.h` takes a long time to compile
     */
    template <ComInterface ComType>
    class ComPtr {
    public:
        // ReSharper disable CppNonExplicitConvertingConstructor

        ComPtr() = default;

        template <typename OtherType>
        ComPtr(OtherType* new_ptr);

        ComPtr(const ComPtr& other);
        ComPtr& operator=(const ComPtr& other);

        ComPtr(ComPtr&& old) noexcept;
        ComPtr& operator=(ComPtr&& old) noexcept;

        // ReSharper restore CppNonExplicitConvertingConstructor

        ~ComPtr();

        [[nodiscard]] ComType** operator&();

        [[nodiscard]] ComType* operator*() const;

        [[nodiscard]] ComType* operator->() const;

        // ReSharper disable once CppNonExplicitConversionOperator
        [[nodiscard]] operator ComType*() const;

        template <typename QueryType>
        [[nodiscard]] ComPtr<QueryType> as() const;

    private:
        ComType* ptr{nullptr};

        void add_ref();

        void remove_ref();
    };

    template <ComInterface ComType>
    template <typename OtherType>
    ComPtr<ComType>::ComPtr(OtherType* new_ptr) : ptr{new_ptr} {
        add_ref();
    }

    template <ComInterface ComType>
    ComPtr<ComType>::ComPtr(const ComPtr& other) : ptr{other.ptr} {
        add_ref();
    }

    template <ComInterface ComType>
    ComPtr<ComType>& ComPtr<ComType>::operator=(const ComPtr& other) {
        if(this == &other) {
            return *this;
        }

        this->~ComPtr();

        ptr = other.ptr;
        add_ref();

        return *this;
    }

    template <ComInterface ComType>
    ComPtr<ComType>::ComPtr(ComPtr&& old) noexcept : ptr{old.ptr} {
        add_ref();

        old.remove_ref();
        old.ptr = nullptr;
    }

    template <ComInterface ComType>
    ComPtr<ComType>& ComPtr<ComType>::operator=(ComPtr&& old) noexcept {
        this->~ComPtr();

        ptr = old.ptr;
        add_ref();

        old.remove_ref();
        old.ptr = nullptr;

        return *this;
    }

    template <ComInterface ComType>
    ComPtr<ComType>::~ComPtr() {
        remove_ref();
    }

    template <ComInterface ComType>
    ComType** ComPtr<ComType>::operator&() {
        return &ptr;
    }

    template <ComInterface ComType>
    ComType* ComPtr<ComType>::operator*() const {
        return ptr;
    }

    template <ComInterface ComType>
    ComType* ComPtr<ComType>::operator->() const {
        return ptr;
    }

    template <ComInterface ComType>
    ComPtr<ComType>::operator ComType*() const {
        return ptr;
    }

    template <ComInterface ComType>
    template <typename QueryType>
    ComPtr<QueryType> ComPtr<ComType>::as() const {
        QueryType* query_obj{nullptr};
        const auto result = ptr->QueryInterface(&query_obj);
        if(FAILED(result)) {
            return {};
        }

        return query_obj;
    }

    template <ComInterface ComType>
    void ComPtr<ComType>::add_ref() {
        if(ptr != nullptr) {
            ptr->AddRef();
        }
    }

    template <ComInterface ComType>
    void ComPtr<ComType>::remove_ref() {
        if(ptr != nullptr) {
            ptr->Release();
        }
    }
} // namespace sanity::engine
