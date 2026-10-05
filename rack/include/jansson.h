/*
 * JSON values, with the functions Rack modules use from the Jansson
 * library (they keep their settings as JSON). The Oroboro Modular SDK's
 * own implementation (rack/src/rack_json.cpp), to Jansson's interface: the
 * values a module makes and reads here are these, not Jansson's.
 */
#ifndef OROBORO_RACK_JANSSON_H
#define OROBORO_RACK_JANSSON_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { JSON_OBJECT, JSON_ARRAY, JSON_STRING, JSON_INTEGER, JSON_REAL, JSON_TRUE, JSON_FALSE, JSON_NULL } json_type;

typedef struct json_t json_t;
typedef long long json_int_t;
#define JSON_INTEGER_FORMAT "lld"
#define JSON_INTEGER_IS_LONG_LONG 1

#define JSON_ERROR_TEXT_LENGTH 160
#define JSON_ERROR_SOURCE_LENGTH 80
typedef struct json_error_t {
	int line;
	int column;
	int position;
	char source[JSON_ERROR_SOURCE_LENGTH];
	char text[JSON_ERROR_TEXT_LENGTH];
} json_error_t;

json_type json_typeof(const json_t *json);
#define json_is_object(json) ((json) && json_typeof(json) == JSON_OBJECT)
#define json_is_array(json) ((json) && json_typeof(json) == JSON_ARRAY)
#define json_is_string(json) ((json) && json_typeof(json) == JSON_STRING)
#define json_is_integer(json) ((json) && json_typeof(json) == JSON_INTEGER)
#define json_is_real(json) ((json) && json_typeof(json) == JSON_REAL)
#define json_is_number(json) (json_is_integer(json) || json_is_real(json))
#define json_is_true(json) ((json) && json_typeof(json) == JSON_TRUE)
#define json_is_false(json) ((json) && json_typeof(json) == JSON_FALSE)
#define json_boolean_value json_is_true
#define json_is_boolean(json) (json_is_true(json) || json_is_false(json))
#define json_is_null(json) ((json) && json_typeof(json) == JSON_NULL)

json_t *json_object(void);
json_t *json_array(void);
json_t *json_string(const char *value);
json_t *json_stringn(const char *value, size_t len);
json_t *json_string_nocheck(const char *value);
json_t *json_integer(json_int_t value);
json_t *json_real(double value);
json_t *json_true(void);
json_t *json_false(void);
#define json_boolean(val) ((val) ? json_true() : json_false())
json_t *json_null(void);

json_t *json_incref(json_t *json);
void json_decref(json_t *json);

/* objects */
size_t json_object_size(const json_t *object);
json_t *json_object_get(const json_t *object, const char *key);
int json_object_set_new(json_t *object, const char *key, json_t *value);
int json_object_set(json_t *object, const char *key, json_t *value);
int json_object_set_nocheck(json_t *object, const char *key, json_t *value);
int json_object_set_new_nocheck(json_t *object, const char *key, json_t *value);
int json_object_del(json_t *object, const char *key);
int json_object_clear(json_t *object);
int json_object_update(json_t *object, json_t *other);
void *json_object_iter(json_t *object);
void *json_object_iter_at(json_t *object, const char *key);
void *json_object_key_to_iter(const char *key);
void *json_object_iter_next(json_t *object, void *iter);
const char *json_object_iter_key(void *iter);
json_t *json_object_iter_value(void *iter);
int json_object_iter_set_new(json_t *object, void *iter, json_t *value);

#define json_object_foreach(object, key, value) \
	for (key = json_object_iter_key(json_object_iter(object)); \
	     key && (value = json_object_iter_value(json_object_key_to_iter(key))); \
	     key = json_object_iter_key(json_object_iter_next(object, json_object_key_to_iter(key))))

#define json_object_foreach_safe(object, n, key, value) \
	for (key = json_object_iter_key(json_object_iter(object)), n = json_object_iter_next(object, json_object_key_to_iter(key)); \
	     key && (value = json_object_iter_value(json_object_key_to_iter(key))); \
	     key = json_object_iter_key(n), n = json_object_iter_next(object, json_object_key_to_iter(key)))

/* arrays */
size_t json_array_size(const json_t *array);
json_t *json_array_get(const json_t *array, size_t index);
int json_array_set_new(json_t *array, size_t index, json_t *value);
int json_array_set(json_t *array, size_t index, json_t *value);
int json_array_append_new(json_t *array, json_t *value);
int json_array_append(json_t *array, json_t *value);
int json_array_insert_new(json_t *array, size_t index, json_t *value);
int json_array_insert(json_t *array, size_t index, json_t *value);
int json_array_remove(json_t *array, size_t index);
int json_array_clear(json_t *array);
int json_array_extend(json_t *array, json_t *other);

#define json_array_foreach(array, index, value) \
	for (index = 0; index < json_array_size(array) && (value = json_array_get(array, index)); index++)

/* values */
const char *json_string_value(const json_t *string);
size_t json_string_length(const json_t *string);
json_int_t json_integer_value(const json_t *integer);
double json_real_value(const json_t *real);
double json_number_value(const json_t *json);
int json_string_set(json_t *string, const char *value);
int json_string_setn(json_t *string, const char *value, size_t len);
int json_integer_set(json_t *integer, json_int_t value);
int json_real_set(json_t *real, double value);

int json_equal(const json_t *value1, const json_t *value2);
json_t *json_copy(json_t *value);
json_t *json_deep_copy(const json_t *value);

/* made and taken apart by a format ("{s:i, s:[f, f]}") */
#define JSON_VALIDATE_ONLY 0x1
#define JSON_STRICT 0x2

json_t *json_pack(const char *fmt, ...);
json_t *json_pack_ex(json_error_t *error, size_t flags, const char *fmt, ...);
json_t *json_vpack_ex(json_error_t *error, size_t flags, const char *fmt, va_list ap);
int json_unpack(json_t *root, const char *fmt, ...);
int json_unpack_ex(json_t *root, json_error_t *error, size_t flags, const char *fmt, ...);
int json_vunpack_ex(json_t *root, json_error_t *error, size_t flags, const char *fmt, va_list ap);

/* reading */
#define JSON_REJECT_DUPLICATES 0x1
#define JSON_DISABLE_EOF_CHECK 0x2
#define JSON_DECODE_ANY 0x4
#define JSON_DECODE_INT_AS_REAL 0x8
#define JSON_ALLOW_NUL 0x10

json_t *json_loads(const char *input, size_t flags, json_error_t *error);
json_t *json_loadb(const char *buffer, size_t buflen, size_t flags, json_error_t *error);
json_t *json_loadf(FILE *input, size_t flags, json_error_t *error);
json_t *json_load_file(const char *path, size_t flags, json_error_t *error);

/* writing */
#define JSON_MAX_INDENT 0x1F
#define JSON_INDENT(n) ((n) & JSON_MAX_INDENT)
#define JSON_COMPACT 0x20
#define JSON_ENSURE_ASCII 0x40
#define JSON_SORT_KEYS 0x80
#define JSON_PRESERVE_ORDER 0x100
#define JSON_ENCODE_ANY 0x200
#define JSON_ESCAPE_SLASH 0x400
#define JSON_REAL_PRECISION(n) (((n) & 0x1F) << 11)
#define JSON_EMBED 0x10000

/* The text, to free() when done. */
char *json_dumps(const json_t *json, size_t flags);
int json_dumpf(const json_t *json, FILE *output, size_t flags);
int json_dump_file(const json_t *json, const char *path, size_t flags);

#ifdef __cplusplus
}
#endif

#endif
