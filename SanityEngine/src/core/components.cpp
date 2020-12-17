#include "core/components.hpp"

#include "entt/entity/registry.hpp"

namespace sanity::engine {
    static Rx::Concurrency::Atomic<Uint64> next_id = 0;

    SanityEngineEntity::SanityEngineEntity() { id = next_id.fetch_add(1); }

    void SanityEngineEntity::add_tag(const Rx::String& tag) {
        if(auto* num_stacks = tags.find(tag); num_stacks != nullptr) {
            (*num_stacks)++;
        } else {
            tags.insert(tag, 1);
        }
    }

    void SanityEngineEntity::add_stacks_of_tag(const Rx::String& tag, const Int32 num_stacks) {
        if(auto* cur_num_stacks = tags.find(tag); cur_num_stacks != nullptr) {
            (*cur_num_stacks) = num_stacks;
        } else {
            tags.insert(tag, num_stacks);
        }
    }

    void SanityEngineEntity::remove_tag(const Rx::String& tag) {
        if(auto* num_stacks = tags.find(tag); num_stacks != nullptr) {
            (*num_stacks)--;
        }
    }

    void SanityEngineEntity::remove_num_tags(const Rx::String& tag, const Uint32 num_stacks) {
        if(auto* cur_num_stacks = tags.find(tag); cur_num_stacks != nullptr) {
            (*cur_num_stacks) -= num_stacks;
        }
    }

    glm::mat4 TransformComponent::get_world_matrix(const entt::registry& registry) const {
        const auto local_matrix = transform.to_matrix();

        if(parent) {
            const auto& parent_transform = registry.get<TransformComponent>(*parent);
            return local_matrix * parent_transform.get_world_matrix(registry);

        } else {
            return local_matrix;
        }
    }
} // namespace sanity::engine
