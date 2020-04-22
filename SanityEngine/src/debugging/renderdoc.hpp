#pragma once

#include <memory>
#include <string>

#include "renderdoc_app.h"

std::unique_ptr<RENDERDOC_API_1_3_0> load_renderdoc(const std::string& renderdoc_dll_path);
