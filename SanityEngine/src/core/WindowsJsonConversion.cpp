#include "core/WindowsJsonConversion.hpp"

#include <Windows.h>
#include <combaseapi.h>

#include "core/RexJsonConversion.hpp"
#include "nlohmann/json.hpp"
#include "rx/core/log.h"
#include "windows/windows_helpers.hpp"

RX_LOG("JsonConversion", logger);

void to_json(nlohmann::json& j, const _GUID& g) {
    OLECHAR* guid_string;
    const auto result = StringFromCLSID(g, &guid_string);
    if(FAILED(result)) {
        logger->error("Could not convert GUID to string: %s", to_string(result));
        return;
    }

    const auto thicc_string = Rx::WideString{reinterpret_cast<Rx::Uint16*>(guid_string)};
    const auto string = thicc_string.to_utf8();

    to_json(j, string);
}

void from_json(const nlohmann::json& j, _GUID& g) {
    Rx::String guid_string;
    from_json(j, guid_string);

    const auto thicc_string = guid_string.to_utf16();

    const auto result = CLSIDFromString(reinterpret_cast<LPCOLESTR>(thicc_string.data()), &g);
    if(FAILED(result)) {
        logger->error("Could not convert string '%s' into a GUID: %s", guid_string, to_string(result));
        return;
    }
}
