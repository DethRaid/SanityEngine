#pragma once

#include "adapters/rex/rex_wrapper.hpp"
#include "core/VectorHandle.hpp"
#include "renderer/handles.hpp"
#include "renderer/rhi/resources.hpp"
#include "rx/core/concurrency/mutex.h"
#include "rx/core/function.h"
#include "rx/core/ptr.h"

namespace sanity::engine {
    namespace renderer {
        class Renderer;
    }

    class AssetLoader;

    template <typename AssetType>
    struct AssetLoadResult {
        bool is_complete{false};

        bool succeeded{false};

        Rx::Ptr<AssetType> asset{};
    };

    using ImageLoadResult = AssetLoadResult<renderer::TextureHandle>;

    template <typename AssetType>
    class AssetLoadResultHandle : public VectorHandle<AssetLoadResult<AssetType>> {
    public:
        AssetLoadResultHandle(AssetLoader& asset_loader_in, Rx::Vector<AssetLoadResult<AssetType>>* container_in, Size index_in);

        virtual ~AssetLoadResultHandle() = default;

    protected:
        AssetLoader* asset_loader;
    };

    class ImageLoadResultHandle : public AssetLoadResultHandle<renderer::TextureHandle> {
    public:
        ImageLoadResultHandle(AssetLoader& asset_loader_in, Rx::Vector<ImageLoadResult>* container_in, Size index_in);

        ~ImageLoadResultHandle() override;
    };

    /*!
     * \brief A class to keep track of asset loading tasks
     */
    class AssetLoader {
    public:
        explicit AssetLoader(renderer::Renderer* renderer_in);

        [[nodiscard]] ImageLoadResultHandle load_image(const std::filesystem::path& path,
                                                       const Rx::Function<void(const ImageLoadResult&)>& on_complete);

        void release_image_at_idx(Size idx);

    private:
        Rx::Concurrency::Mutex image_load_results_mutex;
        Rx::Vector<ImageLoadResult> image_load_results;
        Rx::Vector<bool> image_load_result_availability;

        renderer::Renderer* renderer{nullptr};
    };

} // namespace sanity::engine
