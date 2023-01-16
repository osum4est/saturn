#ifndef SATURN_STAGE_H
#define SATURN_STAGE_H

#include "ecs_types.h"

namespace saturn {

typedef stage_id stage;

namespace stages {

static stage pre_update = _::ecs_core::create_stage_id();
static stage update = _::ecs_core::create_stage_id();
static stage post_update = _::ecs_core::create_stage_id();

} // namespace stages

} // namespace saturn

#endif
