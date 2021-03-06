#pragma once
#ifndef FGEN_H
#define FGEN_H

#include <stdlib.h>
#include <ubermath/ubermath.h>

#include "chaos/core/body.h"

#define FORCE_VECTOR_INIT_CAPACITY 4
#define FORCE_VECTOR_RESIZE_FACTOR 2

struct Gravity {
  vec3 gravity_direction;
};

void gravity_init(struct Gravity* gravity, vec3 gravity_direction);
void gravity_update_force(struct Gravity* gravity, struct RigidBody* body, float duration);

struct Spring {
  vec3 connection_point;
  vec3 other_connection_point;
  struct RigidBody* other;
  float spring_constant;
  float rest_length;
};

void spring_init(struct Spring* spring, vec3 local_connection_point, struct RigidBody* other, vec3 other_connection_point, float spring_constant, float rest_length);
void spring_update_force(struct Spring* spring, struct RigidBody* body, float duration);

struct Explosion {
  vec3 detonation;
  float implosion_max_radius;
  float implosion_min_radius;
  float implosion_duration;
  float implosion_force;
  float shockwave_speed;
  float shockwave_thickness;
  float peak_concussion_force;
  float concussion_duration;
  float peak_convection_force;
  float chimney_radius;
  float chimney_height;
  float convection_duration;
};

//void explosion_init(struct Explosion* explosion);
//void explosion_update_force(struct Explosion* explosion, struct RigidBody* body, real duration);

struct Aero {
  mat3 tensor;
  vec3 position;
  // NOTE: Might be dangerous watch
  vec3* wind_speed;
};

void aero_init(struct Aero* aero, mat3 tensor, vec3 position, vec3* wind_speed);
void aero_update_force(struct Aero* aero, struct RigidBody* body, float duration);
void aero_update_force_from_tensor(struct Aero* aero, struct RigidBody* body, float duration, mat3 tensor);

struct AeroControl {
  struct Aero aero;
  mat3 max_tensor;
  mat3 min_tensor;
  float control_setting;
};

void aero_control_init(struct AeroControl* aero_control, mat3 base, mat3 min, mat3 max, vec3 position, vec3* wind_speed);
void aero_control_delete(struct AeroControl* aero_control);
mat3 aero_control_get_tensor(struct AeroControl* aero_control);
void aero_control_set_control(struct AeroControl* aero_control, float value);
void aero_control_update_force(struct AeroControl* aero_control, struct RigidBody* body, float duration);

struct AngledAero {
  struct Aero aero;
  quat orientation;
};

//void angled_aero_init(struct AngledAero* angled_aero, real* tensor, real* position, real* windspeed);
//void angled_aero_set_orientation(real* quat);
//void angled_aero_update_force(struct AngledAero* angled_aero, struct RigidBody* body, real duration);

struct Buoyancy {
  float max_depth;
  float volume;
  float water_height;
  float liquid_density;
  vec3 centre_of_bouyancy;
};

// Note: Default liquid density is 1000.0f
void buoyancy_init(struct Buoyancy* buoyancy, vec3 c_of_b, float max_depth, float volume, float water_height, float liquid_density);
void buoyancy_update_force(struct Buoyancy* buoyancy, struct RigidBody* body, float duration);

//////////////////////////////////////////////////////

enum ForceType { GRAVITY,
                 SPRING,
                 EXPLOSION,
                 AERO,
                 AERO_CONTROL,
                 ANGLED_AERO,
                 BUOYANCY };

union Force {
  struct Gravity gravity;
  struct Spring spring;
  struct Explosion explosion;
  struct Aero aero;
  struct AeroControl aero_control;
  struct AngledAero angled_aero;
  struct Buoyancy buoyancy;
};

struct ForceGenerator {
  enum ForceType force_type;
  union Force force;
  void (*update_force)(void*, struct RigidBody*, float);
};

struct ForceRegistration {
  struct RigidBody* body;
  struct ForceGenerator* fg;
};

//////////////////////////////////////////////////////

struct ForceVector {
  size_t size;
  size_t capacity;
  void* items;
};

// TODO: Move out of header
static inline void force_vector_init(struct ForceVector* vector, size_t memory_size);
static inline void force_vector_delete(struct ForceVector* vector);
static inline size_t force_vector_size(struct ForceVector* vector);
static inline void force_vector_resize(struct ForceVector* vector, size_t capacity);
static inline void force_vector_push_back(struct ForceVector* vector, void* item);
static inline void* force_vector_get(struct ForceVector* vector, size_t index);
static inline void force_vector_remove(struct ForceVector* vector, size_t index);
static inline void force_vector_clear(struct ForceVector* vector);

static inline void force_vector_init(struct ForceVector* vector, size_t memory_size) {
  vector->size = 0;
  vector->capacity = FORCE_VECTOR_INIT_CAPACITY;
  vector->items = malloc(sizeof(struct ForceRegistration) * FORCE_VECTOR_INIT_CAPACITY);
}

static inline void force_vector_delete(struct ForceVector* vector) {
  free(vector->items);
}

static inline size_t force_vector_size(struct ForceVector* vector) {
  return vector->size;
}

static inline void force_vector_resize(struct ForceVector* vector, size_t capacity) {
  void* new_items = realloc((char*)vector->items, sizeof(struct ForceRegistration) * capacity);

  if (new_items) {
    vector->items = new_items;
    vector->capacity = capacity;
  }
}

static inline void force_vector_push_back(struct ForceVector* vector, void* item) {
  if (vector->capacity == vector->size)
    force_vector_resize(vector, vector->capacity * FORCE_VECTOR_RESIZE_FACTOR);

  memcpy((char*)vector->items + (sizeof(struct ForceRegistration) * vector->size), item, sizeof(struct ForceRegistration));
  vector->size++;
}

static inline void* force_vector_get(struct ForceVector* vector, size_t index) {
  if (index >= 0 && index < vector->size)
    return (char*)vector->items + (sizeof(struct ForceRegistration) * index);
  return NULL;
}

static inline void force_vector_remove(struct ForceVector* vector, size_t index) {
  if (index < 0 || index >= vector->size)
    return;
  memmove((char*)vector->items + (sizeof(struct ForceRegistration) * index), (char*)vector->items + (sizeof(struct ForceRegistration) * (index + 1)), sizeof(struct ForceRegistration) * (vector->size - (index + 1)));

  vector->size--;

  if (vector->size > 0 && vector->size == vector->capacity / 4)
    force_vector_resize(vector, vector->capacity / 2);
}

static inline void force_vector_clear(struct ForceVector* vector) {
  vector->size = 0;
  vector->capacity = FORCE_VECTOR_INIT_CAPACITY;
  force_vector_resize(vector, FORCE_VECTOR_INIT_CAPACITY);
}

//////////////////////////////////////////////////////

struct ForceRegistry {
  struct ForceVector registrations;
};

void force_registry_update_forces(struct ForceRegistry* force_registry, float duration);
void force_registry_add(struct ForceRegistry* force_registry, struct RigidBody* body, struct ForceGenerator* fg);
void force_registry_remove(struct ForceRegistry* force_registry, struct RigidBody* body, struct ForceGenerator* fg);
void force_registry_clear(struct ForceRegistry* force_registry);

#endif  // FGEN_H
