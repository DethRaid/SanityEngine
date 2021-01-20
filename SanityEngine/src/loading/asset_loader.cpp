#include "asset_loader.hpp"

#include "loading/image_loading.hpp"
#include "rx/core/concurrency/scope_lock.h "
#include "tracy/Tracy.hpp"

namespace sanity::engine {
    template <typename AssetType>
    AssetLoadResultHandle<AssetType>::AssetLoadResultHandle(AssetLoader& asset_loader_in,
                                                            Rx::Vector<AssetLoadResult<AssetType>>* container_in,
                                                            const Size index_in)
        : VectorHandle<AssetLoadResult<AssetType>>{container_in, index_in}, asset_loader{&asset_loader_in} {}

    ImageLoadResultHandle::ImageLoadResultHandle(AssetLoader& asset_loader_in,
                                                 Rx::Vector<ImageLoadResult>* container_in,
                                                 const Size index_in)
        : AssetLoadResultHandle{asset_loader_in, container_in, index_in} {}

    ImageLoadResultHandle::~ImageLoadResultHandle() { asset_loader->release_image_at_idx(get_index()); }

    AssetLoader::AssetLoader(renderer::Renderer* renderer_in) : renderer{renderer_in} {}

    ImageLoadResultHandle AssetLoader::load_image(const std::filesystem::path& path, const Rx::Function<void(const ImageLoadResult&)>& on_complete) {
        ZoneScoped;
        auto idx = Size{0};

        {
            ZoneScopedN("Initialize results");

            Rx::Concurrency::ScopeLock _{image_load_results_mutex};

            auto can_reuse_result = false;
            image_load_result_availability.each_fwd([&](const bool& is_free) {
                if(is_free) {
                    can_reuse_result = true;

                    return RX_ITERATION_STOP;
                }

                idx++;

                return RX_ITERATION_CONTINUE;
            });

            if(can_reuse_result) {
                image_load_results[idx] = {};
                image_load_result_availability[idx] = false;

            } else {
                idx = image_load_results.size();
                image_load_results.emplace_back();
                image_load_result_availability.push_back(false);
            }
        }

        auto accessor = ImageLoadResultHandle{*this, &image_load_results, idx};

        // TODO: Eventually we'll probably want to make this method async, but that's hard
        auto final_result = ImageLoadResult{};

        const auto handle_maybe = load_image_to_gpu(path, *renderer);
        if(handle_maybe) {
            final_result.is_complete = true;
            final_result.succeeded = true;
            final_result.asset = Rx::make_ptr<renderer::TextureHandle>(RX_SYSTEM_ALLOCATOR, *handle_maybe);
        }

        {
            Rx::Concurrency::ScopeLock _{image_load_results_mutex};
            image_load_results[idx] = Rx::Utility::move(final_result);

            ZoneScopedN("on_complete");
            on_complete(image_load_results[idx]);
        }

        return accessor;
    }

    void AssetLoader::release_image_at_idx(const Size idx) {
        Rx::Concurrency::ScopeLock _{image_load_results_mutex};
        image_load_result_availability[idx] = true;
    }
} // namespace sanity::engine
