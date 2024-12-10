/* Behold, macro horrors!
 *
 * This is an attempt to create a generic dynamic array setup.
 * Source files use USE_DYNAMIC_ARRAY() with a type of choice,
 * and the corresponding function definitions will be pasted in
 * by the preprocessor.
 *
 * Then, dynamic arrays can be utilized with the provided macros:
 *   - DA(type) for type usage, e.g. DA(float) x;
 *   - DA_INIT(type, arr)
 *   - DA_GET(arr, index)
 *   - DA_SET(type, arr, index, value)
 *   - DA_ADD(type, arr, value)
 *   - DA_FREE(type, arr)
 *
 * which will call the respective macro-generated functions.
 */

#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stddef.h> // for NULL
#include <stdlib.h> // for realloc

#define INITIAL_CAPACITY 16

#define da_struct_name(type) DynamicArray_ ## type

#define da_init_function_name(type) Init_DynamicArray_ ## type
#define da_add_function_name(type)   Add_DynamicArray_ ## type
#define da_set_function_name(type)   Set_DynamicArray_ ## type
#define da_free_function_name(type) Free_DynamicArray_ ## type

#define da_struct_definition(type) \
  struct da_struct_name(type) {    \
    int count;                     \
    int capacity;                  \
    type *data;                    \
  };

#define da_init_definition(type)          \
  static void                             \
  da_init_function_name(type)(            \
      struct da_struct_name(type) *array  \
  )                                       \
  {                                       \
    array->count = 0;                     \
    array->capacity = 0;                  \
    array->data = NULL;                   \
  }

#define da_add_definition(type)                                      \
  static void                                                        \
  da_add_function_name(type)(                                        \
      struct da_struct_name(type) *array,                            \
      type value                                                     \
  )                                                                  \
  {                                                                  \
    if (array->capacity < array->count + 1) {                        \
      array->capacity = (array->capacity < INITIAL_CAPACITY)         \
                          ? INITIAL_CAPACITY                         \
                          : array->capacity * 2;                     \
      array->data = realloc(array->data,                             \
                            array->capacity * sizeof(*array->data)); \
    }                                                                \
                                                                     \
    array->data[array->count++] = value;                             \
  }

#define da_set_definition(type)                                      \
  static void                                                        \
  da_set_function_name(type)(                                        \
      struct da_struct_name(type) *array,                            \
      int index,                                                     \
      type value                                                     \
  )                                                                  \
  {                                                                  \
    if (array->capacity < array->count + 1) {                        \
      array->capacity = (array->capacity < INITIAL_CAPACITY)         \
                          ? INITIAL_CAPACITY                         \
                          : array->capacity * 2;                     \
      array->data = realloc(array->data,                             \
                            array->capacity * sizeof(*array->data)); \
    }                                                                \
                                                                     \
    array->data[index] = value;                                      \
  }

#define da_free_definition(type)           \
  static void da_free_function_name(type)( \
      struct da_struct_name(type) *array   \
  )                                        \
  {                                        \
    free(array->data);                     \
    da_init_function_name(type)(array);    \
  }

// Pastes all definitions
#define USE_DYNAMIC_ARRAY(type) \
  da_struct_definition(type)    \
  da_init_definition(type)      \
  da_add_definition(type)       \
  da_set_definition(type)       \
  da_free_definition(type)

#define DA(type) struct da_struct_name(type)
#define DA_INIT(type, arr) da_init_function_name(type)(&arr)
#define DA_GET(arr, index) (arr.data[index])
#define DA_SET(type, arr, index, value) da_set_function_name(type)(&arr, index, value)
#define DA_ADD(type, arr, value) da_add_function_name(type)(&arr, value)
#define DA_FREE(type, arr) da_free_function_name(type)(&arr)

#endif
