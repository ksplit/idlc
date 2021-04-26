#ifndef IDLC_GENERATION_C_SPECIFIERS_H

#include "../frontend/analysis.h"
#include "generation.h"

namespace idlc {
    void populate_c_type_specifiers(rpc_vec_view rpcs, global_vec_view globals, projection_vec_view projections);
}

#endif