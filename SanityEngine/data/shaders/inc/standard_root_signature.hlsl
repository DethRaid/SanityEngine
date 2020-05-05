struct Camera {
    float4x4 view;
    float4x4 projection;
    float4x4 previous_view;
    float4x4 previous_projection;
};

struct Light {
    uint type;
    float3 color;
    float3 direction;
    float angular_size;
};

/*!
 * \brief Point sampler you can use to sample any texture
 */
SamplerState point_sampler : register(s0, space0);

/*!
 * \brief Bilinear sampler you can use to sample any texture
 */
SamplerState bilinear_sampler : register(s0, space1);

/*!
 * \brief Trilinear sampler you can use to sample any texture
 */
SamplerState trilinear_sampler : register(s0, space2);

struct StandardPushConstants {
    /*!
     * \brief Index of the camera that will render this draw
     */
    uint camera_index;

    /*!
     * \brief Index of the material data for the current draw
     */
    uint material_index;
} constants;

/*!
 * \brief Array of all the materials
 */
StructuredBuffer<Camera> cameras : register(t0);

/*!
 * \brief Array of all the materials
 */
StructuredBuffer<MaterialData> material_buffer : register(t1);

/*!
 * \brief Array of all the lights in the scene
 *
 * When I get forward+ rendering working, there will be a separate buffer for which lights affect which object
 */
StructuredBuffer<Light> lights : register(t2);

/*!
 * \brief Array of all the textures that are available for a shader to sample from
 */
Texture2D textures[] : register(t2);
