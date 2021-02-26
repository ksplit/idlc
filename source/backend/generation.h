#ifndef IDLC_BACKEND_GENERATION_H
#define IDLC_BACKEND_GENERATION_H

#include "../frontend/analysis.h"

namespace idlc {
    using projection_vec_view = gsl::span<const gsl::not_null<projection*>>;
    void generate(rpc_vec_view rpcs);
}

#endif
