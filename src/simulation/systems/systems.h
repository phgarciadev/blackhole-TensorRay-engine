#ifndef SIMULATION_SYSTEMS_H
#define SIMULATION_SYSTEMS_H

#include "engine/ecs/ecs.h"

void gravity_system_update(bhs_world_handle world, double dt);
void orbital_integrator_system_update(bhs_world_handle world, double dt);

#endif
