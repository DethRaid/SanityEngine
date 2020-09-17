#pragma once

#include "rx/core/vector.h"
#include "rx/core/ptr.h"

#include "rx/core/string.h"

namespace environment {
    
    struct DensityMapPipelineStep {

    };

    /// <summary>
    /// Loads data from a texture
    /// 
    /// </summary>
    struct LoadTextureStep : public DensityMapPipelineStep {
        LoadTextureStep(const Rx::String& texture_name);
    };

    class DensityMapPipeline {
    private:
        Rx::Vector<Rx::Ptr<DensityMapPipelineStep>> steps;
    };
} // namespace environment
