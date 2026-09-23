#include "bixolon_backend.h"

const char* BixolonBackend::name() const
{
    return skipMotionUnitsCommand_ ? "BIXOLON_BK" : "BIXOLON_SRP";
}
