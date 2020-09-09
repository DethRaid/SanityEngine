#include "Generators/RuntimeClasses/RuntimeClassGenerator.hpp"
#include "RexWrapper.hpp"
#include "Types.hpp"
#include "rx/core/log.h"

RX_LOG("SanityEngine.Codegen", logger);

/*!
 * \brief Scans all .hpp files in a provided directory
 *
 * Usage: `SanityEngine.Codegen <C++ dir> <IDL dir>`
 */
int main(const Sint32 argc, const char** argv) {
    auto return_code = Sint32{0};
    auto rex_wrapper = rex::Wrapper{};

    logger->info("HELLO HUMAN");

    if(argc != 3) {
        logger->error("Wrong number of command-line parameters. Usage:\n\n\tSanityEngine.Codegen <C++ directory> <IDL directory>");
        return_code = -1;

    } else {
        const auto cpp_directory = Rx::String{argv[1]};
        const auto idl_directory = Rx::String{argv[2]};

        GenerateRuntimeClasses(cpp_directory, idl_directory);
    }

    logger->warning("REMAIN INDOORS");

    return return_code;
}
