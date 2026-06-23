#ifndef TYPES_H
#define TYPES_H

typedef struct {
    double x;
    double y;
} Vector2D;

typedef struct {
    int id;
    Vector2D position;
    Vector2D velocity;
    double radius;
    double mass;
} Particle;

typedef struct {
    double x;
    double y;
    double width;
    double height;
} AABB;

typedef struct {
    int num_particles;
    double time_step;
    AABB bounds;
} SimulationState;

#endif