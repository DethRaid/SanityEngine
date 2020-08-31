#include <stdlib.h> // abort

#include "rx/core/log.h" // RX_LOG, rx::log
#include "rx/core/abort.h" // rx::abort

RX_LOG("assert", logger);

namespace Rx {

[[noreturn]]
void assert_message(const char* _expression,
  const SourceLocation& _source_location, const char* _message, bool _truncated)
{
  // Write the assertion failure to the logger.
  logger->error("Assertion failed: %s (%s:%d %s) \"%s\"%s",
    _expression, _source_location.file(), _source_location.line(),
    _source_location.function(), _message, _truncated ? "... [truncated]" : "");

  abort(_truncated ? "%s... [truncated]" : "%s", _message);
}

} // namespace rx
