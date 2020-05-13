﻿#include "sanity_engine.hpp"

#define STB_IMAGE_IMPLEMENTATION

#include <filesystem>
#include <glm/ext/quaternion_trigonometric.inl>

#include <GLFW/glfw3.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <stb_image.h>

#include "core/abort.hpp"
#include "loading/image_loading.hpp"
#include "renderer/standard_material.hpp"
#include "rhi/render_device.hpp"
#include "ui/fps_display.hpp"
#include "ui/ui_components.hpp"

struct AtmosphereMaterial {
    glm::vec3 sun_vector;
};

int main() {
    Settings settings{};
    // settings.enable_gpu_crash_reporting = true;

    SanityEngine engine{settings};

    engine.run();

    spdlog::warn("REMAIN INDOORS");
}

static void error_callback(const int error, const char* description) { spdlog::error("{} (GLFW error {}}", description, error); }

static void key_func(GLFWwindow* window, const int key, int /* scancode */, const int action, const int mods) {
    auto* input_manager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

    input_manager->on_key(key, action, mods);
}

SanityEngine::SanityEngine(const Settings& settings_in)
    : logger{spdlog::stdout_color_st("SanityEngine")}, settings{settings_in}, input_manager{std::make_unique<InputManager>()} {
    mtr_init("SanityEngine.json");

    MTR_SCOPE("SanityEngine", "SanityEngine");

    // spdlog::set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");
    spdlog::set_pattern("[%n] [%^%l%$] %v");

    logger->set_level(spdlog::level::debug);

    logger->info("HELLO HUMAN");

    {
        MTR_SCOPE("SanityEngine", "glfwInit");
        if(!glfwInit()) {
            critical_error("Could not initialize GLFW");
        }
    }

    glfwSetErrorCallback(error_callback);

    {
        MTR_SCOPE("SanityEngine", "glfwCreateWindow");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(1000, 480, "Sanity Engine", nullptr, nullptr);
        if(window == nullptr) {
            critical_error("Could not create GLFW window");
        }
    }

    logger->info("Created window");

    glfwSetWindowUserPointer(window, input_manager.get());
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetKeyCallback(window, key_func);

    renderer = std::make_unique<renderer::Renderer>(window, settings);
    logger->info("Initialized renderer");

    bve = std::make_unique<BveWrapper>(renderer->get_render_device());

    create_planetary_atmosphere();

    make_frametime_display();
}

SanityEngine::~SanityEngine() {
    glfwDestroyWindow(window);

    glfwTerminate();

    mtr_shutdown();
}

void SanityEngine::run() {
    uint64_t frame_count = 1;

    float last_frame_duration = 0;

    while(!glfwWindowShouldClose(window)) {
        {
            MTR_SCOPE("SanityEngine", "tick");
            const auto frame_start_time = std::chrono::steady_clock::now();
            glfwPollEvents();

            renderer->begin_frame(frame_count);

            // Hackily spawn entities on the first frame, because mesh uploading is hard
            if(frame_count == 1) {
                // renderer->begin_device_capture();

                // TODO: Figure out a good way to submit GPU work before the first frame has begun

                imgui_adapter = std::make_unique<DearImguiAdapter>(window, *renderer);

                create_debug_plane();

                create_flycam_player();

                load_bve_train("data/bve_trains/R46 2014 (8 Car)/Cars/Body/BodyA.b3d");

                load_3d_object("data/trains/BestFriend.obj");
            }

            player_controller->update_player_transform(last_frame_duration);

            imgui_adapter->draw_ui(registry.view<ui::UiComponent>());

            renderer->render_all(registry);

            renderer->end_frame();

            if(frame_count == 1) {
                // renderer->end_device_capture();
            }

            const auto frame_end_time = std::chrono::steady_clock::now();

            const auto microsecond_frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_end_time - frame_start_time)
                                                        .count();
            last_frame_duration = static_cast<float>(static_cast<double>(microsecond_frame_duration) / 1000000.0);

            framerate_tracker.add_frame_time(last_frame_duration);

            if(frame_count % 100 == 0) {
                logger->info("Frame {} stats", frame_count);
                framerate_tracker.log_framerate_stats();
            }

            frame_count++;
        }

        mtr_flush();
    }
}

void SanityEngine::create_debug_plane() {
    const auto vertices = std::vector<StandardVertex>{
        {/* .position = */ {-5, -1, 5}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFF727272, /* .texcoord = */ {}},
        {/* .position = */ {5, -1, -5}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFF727272, /* .texcoord = */ {}},
        {/* .position = */ {5, -1, 5}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFF727272, /* .texcoord = */ {}},
        {/* .position = */ {-5, -1, -5}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFF727272, /* .texcoord = */ {}},
    };

    const auto indices = std::vector<uint32_t>{0, 1, 2, 0, 3, 1};

    auto& device = renderer->get_render_device();
    auto commands = device.create_resource_command_list();

    auto plane_renderable = renderer->create_static_mesh(vertices, indices, *commands);

    device.submit_command_list(std::move(commands));

    const auto plane_entity = registry.create();

    registry.assign<renderer::StaticMeshRenderableComponent>(plane_entity, std::move(plane_renderable));

    logger->info("Created plane");
}

void SanityEngine::create_planetary_atmosphere() {
    const auto atmosphere = registry.create();

    // No need to set parameters, the default light component represents the Earth's sun
    registry.assign<renderer::LightComponent>(atmosphere);
    registry.assign<renderer::AtmosphericSkyComponent>(atmosphere);
}

void SanityEngine::make_frametime_display() {
    const auto frametime_display = registry.create();
    registry.assign<ui::UiComponent>(frametime_display, std::make_unique<ui::FramerateDisplay>(framerate_tracker));
}

void SanityEngine::create_flycam_player() {
    player = registry.create();

    auto& transform = registry.assign<TransformComponent>(player);
    transform.position.z = 5;
    transform.position.y = 2;
    transform.rotation = glm::angleAxis(0.0f, glm::vec3{1, 0, 0});
    registry.assign<renderer::CameraComponent>(player);

    player_controller = std::make_unique<FlycamController>(window, player, registry);

    logger->info("Created flycam");
}

void SanityEngine::load_bve_train(const std::string& filename) {
    const auto success = bve->add_train_to_scene(filename, registry, *renderer);
    if(!success) {
        logger->error("Could not load train file {}", filename);
    }
}

void SanityEngine::load_3d_object(const std::string& filename) {
    const auto* scene = importer.ReadFile(filename, aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_MaxQuality);

    if(scene == nullptr) {
        logger->error("Could not load {}", filename);
        return;
    }

    auto& device = renderer->get_render_device();
    auto command_list = device.create_resource_command_list();

    std::unordered_map<uint32_t, renderer::MaterialHandle> materials;

    // Initial revision: import the root node and hope it's fine
    const auto* node = scene->mRootNode;
    for(uint32_t mesh_idx = 0; mesh_idx < node->mNumMeshes; mesh_idx++) {
        // Get the mesh at this index
        const auto ass_mesh_handle = node->mMeshes[mesh_idx];
        const auto* mesh = scene->mMeshes[ass_mesh_handle];

        // Convert it to our vertex format
        std::vector<StandardVertex> vertices;
        vertices.reserve(mesh->mNumVertices);

        for(uint32_t vert_idx = 0; vert_idx < mesh->mNumVertices; vert_idx++) {
            const auto& position = mesh->mVertices[vert_idx];
            const auto& normal = mesh->mNormals[vert_idx];
            // const auto& tangent = mesh->mTangents[vert_idx];
            // const auto& color = mesh->mColors[0][vert_idx];
            const auto& texcoord = mesh->mTextureCoords[0][mesh_idx];

            vertices.push_back(StandardVertex{.position = {position.x, position.y, position.z},
                                              .normal = {normal.x, normal.y, normal.z},
                                              .texcoord = {texcoord.x, texcoord.y}});
        }

        std::vector<uint32_t> indices;
        indices.reserve(mesh->mNumFaces * 3);
        for(uint32_t face_idx = 0; face_idx < mesh->mNumFaces; face_idx++) {
            const auto& face = mesh->mFaces[face_idx];
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }

        const auto mesh_handle = renderer->create_static_mesh(vertices, indices, *command_list);

        const auto mesh_entity = registry.create();
        auto& mesh_renderer = registry.assign<renderer::StaticMeshRenderableComponent>(mesh_entity);
        mesh_renderer.mesh = mesh_handle;

        if(const auto& mat = materials.find(mesh->mMaterialIndex); mat != materials.end()) {
            mesh_renderer.material = mat->second;

        } else {
            auto& material_store = renderer->get_material_data_buffer();
            const auto material_handle = material_store.get_next_free_material<StandardMaterial>();
            auto& material = material_store.at<StandardMaterial>(material_handle);

            const auto* ass_material = scene->mMaterials[mesh->mMaterialIndex];

            // TODO: Useful logic to select between material formats
            aiString ass_texture_path;
            const auto result = ass_material->GetTexture(aiTextureType_BASE_COLOR, 0, &ass_texture_path);
            if(result == aiReturn_SUCCESS) {
                // Load texture into Sanity Engine and set on material

                if(const auto existing_image_handle = renderer->get_image_handle(ass_texture_path.C_Str())) {
                    material.albedo = *existing_image_handle;

                } else {
                    uint32_t width, height;
                    std::vector<uint8_t> pixels;
                    const auto was_image_loaded = load_image(ass_texture_path.C_Str(), width, height, pixels);
                    if(!was_image_loaded) {
                        logger->warn("Could not load texture {}", ass_texture_path.C_Str());

                        // TODO: Use the no-material pink texture

                        continue;
                    }

                    const auto create_info = rhi::ImageCreateInfo{.name = ass_texture_path.C_Str(),
                                                                  .usage = rhi::ImageUsage::SampledImage,
                                                                  .width = width,
                                                                  .height = height};

                    const auto image_handle = renderer->create_image(create_info, pixels.data());
                    material.albedo = image_handle;
                }

            } else {
                // Get the material base color. Create a renderer texture with this color, set that texture as the albedo
                // If there's no material base color, use a pure white texture
            }
        }
    }

    device.submit_command_list(std::move(command_list));
}
