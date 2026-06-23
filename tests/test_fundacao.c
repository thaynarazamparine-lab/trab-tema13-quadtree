#include <assert.h>
#include <stdio.h>
#include "../include/types.h"
void test_particle_creation() {
    Particle p = {1, {10.0, 10.0}, {0.0, 0.0}, 1.0, 5.0};
    assert(p.id == 1);
    assert(p.position.x == 10.0);
    assert(p.position.y == 10.0);
    assert(p.radius == 1.0);
}
void test_simulation_state() {
    SimulationState state = {1000, 0.016, {0.0, 0.0, 800.0, 600.0}};
    assert(state.num_particles == 1000);
    assert(state.bounds.width == 800.0);
}

int main() {
    test_particle_creation();
    test_simulation_state();
    printf("Todos os testes da fundacao passaram.\n");
    return 0;
}
