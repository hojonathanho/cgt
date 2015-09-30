#pragma once

#include "cgt_common.h"
#include "execution.h"

// void serialize(const cgt::ExecutionGraph* eg);
namespace cgt {

std::string serialize(cgtArray*);
cgtArray* deserializeArray(const std::string&);

std::string serialize(const MemLocation&);
MemLocation deserializeMemLocation(const std::string&);

} // namespace cgt
