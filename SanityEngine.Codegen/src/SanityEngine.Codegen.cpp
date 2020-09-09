#include "RexWrapper.hpp"

#include "rx/core/log.h"

RX_LOG("SanityEngine.Codegen", logger);

int main()
{
    auto rex_wrapper = rex::Wrapper{};

    logger->info("HELLO HUMAN");
    
    logger->warning("REMAIN INDOORS");
}
