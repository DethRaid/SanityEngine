#pragma once

#include "core/Prelude.hpp"
#include "core/types.hpp"
#include "renderer/mesh.hpp"
#include "renderer/rhi/resources.hpp"
#include "rx/core/ptr.h"
#include "rx/core/vector.h"

struct StandardVertex {
    Vec3f location{};
    Vec3f normal{};
    Uint32 color{0xFFFFFFFF};
    Vec2f texcoord{};
};

namespace sanity::engine::renderer {
	class Renderer;
	class MeshDataStore;
    class ResourceCommandList;
    class RenderBackend;

    /*!
     * \brief Binding for a vertex buffer
     */
    struct VertexBufferBinding {
        /*!
         * \brief The buffer to bind
         */
        Buffer buffer;

        /*!
         * \brief Offset in bytes where the relevant data starts
         */
        Uint32 offset;

        /*!
         * \brief Size of a vertex, in bytes
         */
        Uint32 vertex_size;
    };

    class MeshUploader final {
    public:
        explicit MeshUploader(ID3D12GraphicsCommandList4* cmds_in, MeshDataStore* mesh_store_in);

        MeshUploader(const MeshUploader& other) = delete;
        MeshUploader& operator=(const MeshUploader& other) = delete;

        MeshUploader(MeshUploader&& old) noexcept;
        MeshUploader& operator=(MeshUploader&& old) noexcept;

        ~MeshUploader();

        [[nodiscard]] Mesh add_mesh(const Rx::Vector<StandardVertex>& vertices, const Rx::Vector<Uint32>& indices) const;

        void prepare_for_raytracing_geometry_build();

    private:
        enum class State {
            AddVerticesAndIndices,
            BuildRaytracingGeometry,
            Empty,
        };

        State state{State::AddVerticesAndIndices};

        ID3D12GraphicsCommandList4* cmds;
        MeshDataStore* mesh_store;
    };

    class MeshDataStore {
    public:
        MeshDataStore(Renderer& renderer_in, BufferHandle vertex_buffer_in, BufferHandle index_buffer_in);

        MeshDataStore(const MeshDataStore& other) = delete;
        MeshDataStore& operator=(const MeshDataStore& other) = delete;

        MeshDataStore(MeshDataStore&& old) noexcept = default;
        MeshDataStore& operator=(MeshDataStore&& old) noexcept = delete;

        ~MeshDataStore();

        [[nodiscard]] BufferHandle get_vertex_buffer_handle() const;

        [[nodiscard]] BufferHandle get_index_buffer_handle() const;

    	[[nodiscard]] Buffer get_vertex_buffer() const;

    	[[nodiscard]] Buffer get_index_buffer() const;

        /*!
         * \brief Prepares the vertex and index buffers to receive new mesh data
         */
        [[nodiscard]] MeshUploader begin_adding_meshes(ID3D12GraphicsCommandList4* commands);

        void bind_to_command_list(ID3D12GraphicsCommandList4* commands) const;

    private:
        Renderer* renderer;

        BufferHandle vertex_buffer_handle;

        BufferHandle index_buffer_handle;

        Rx::Vector<VertexBufferBinding> vertex_bindings{};

        /*!
         * \brief Index of the byte in the vertex buffer where the next mesh can be uploaded to
         *
         * I'll eventually need a way to unload meshes, but that's more complicated
         */
        Uint32 next_free_vertex_byte{0};

        /*!
         * \brief The offset in the vertex buffer, in vertices, where the next mesh's vertex data should start
         */
        Uint32 next_vertex_offset{0};

        /*!
         * \brief The offset in the index buffer where the next mesh's indices should start
         */
        Uint32 next_index_offset{0};

        friend class MeshUploader;

        /*!
         * \brief Adds new mesh data to the vertex and index buffers. Must be called after `begin_mesh_data_upload` and before
         * `end_mesh_data_upload`
         */
        [[nodiscard]] Mesh add_mesh(const Rx::Vector<StandardVertex>& vertices,
                                    const Rx::Vector<Uint32>& indices,
                                    ID3D12GraphicsCommandList4* commands);
    };
} // namespace sanity::engine::renderer
