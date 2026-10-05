// SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-Oroboro-Rack-Bridge-Exception (rack/LICENSE-EXCEPTION.md)
// JSON values for Rack modules (jansson.h's functions): made, read, changed,
// parsed and written. The Oroboro Modular SDK's own implementation.

#include <jansson.h>

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

/// An object's member: the value, and its key right after it in the same
/// allocation, so that a key's pointer leads back to its member
/// (`json_object_key_to_iter`).
struct Member {
	json_t* value;
	size_t index;
	char* key() { return reinterpret_cast<char*>(this + 1); }
	static Member* of(const char* key) { return reinterpret_cast<Member*>(const_cast<char*>(key)) - 1; }
	static Member* make(const char* key, json_t* value, size_t index) {
		size_t len = std::strlen(key);
		Member* m = static_cast<Member*>(std::malloc(sizeof(Member) + len + 1));
		m->value = value;
		m->index = index;
		std::memcpy(m->key(), key, len + 1);
		return m;
	}
};

} // namespace

struct json_t {
	json_type type;
	size_t refcount;
	std::vector<Member*> members; // an object's, in the order they came
	std::vector<json_t*> items;   // an array's
	std::string text;             // a string's
	json_int_t integer;
	double real;

	json_t(json_type type) : type(type), refcount(1), integer(0), real(0.0) {}
};

namespace {

json_t* const TRUE_VALUE = new json_t(JSON_TRUE);
json_t* const FALSE_VALUE = new json_t(JSON_FALSE);
json_t* const NULL_VALUE = new json_t(JSON_NULL);

bool shared(const json_t* json) { return json == TRUE_VALUE || json == FALSE_VALUE || json == NULL_VALUE; }

Member* find(const json_t* object, const char* key) {
	for (Member* m : object->members)
		if (std::strcmp(m->key(), key) == 0)
			return m;
	return nullptr;
}

void destroy(json_t* json) {
	for (Member* m : json->members) {
		json_decref(m->value);
		std::free(m);
	}
	for (json_t* item : json->items)
		json_decref(item);
	delete json;
}

/// Whether `inner` is `outer` or inside it: putting it there would make a loop.
bool contains(const json_t* outer, const json_t* inner) {
	if (outer == inner)
		return true;
	for (Member* m : outer->members)
		if (contains(m->value, inner))
			return true;
	for (json_t* item : outer->items)
		if (contains(item, inner))
			return true;
	return false;
}

} // namespace

extern "C" {

json_type json_typeof(const json_t* json) { return json ? json->type : JSON_NULL; }

json_t* json_object(void) { return new json_t(JSON_OBJECT); }
json_t* json_array(void) { return new json_t(JSON_ARRAY); }
json_t* json_stringn(const char* value, size_t len) {
	if (!value)
		return nullptr;
	json_t* json = new json_t(JSON_STRING);
	json->text.assign(value, len);
	return json;
}
json_t* json_string(const char* value) { return value ? json_stringn(value, std::strlen(value)) : nullptr; }
json_t* json_string_nocheck(const char* value) { return json_string(value); }
json_t* json_integer(json_int_t value) {
	json_t* json = new json_t(JSON_INTEGER);
	json->integer = value;
	return json;
}
json_t* json_real(double value) {
	if (std::isnan(value) || std::isinf(value))
		return nullptr;
	json_t* json = new json_t(JSON_REAL);
	json->real = value;
	return json;
}
json_t* json_true(void) { return TRUE_VALUE; }
json_t* json_false(void) { return FALSE_VALUE; }
json_t* json_null(void) { return NULL_VALUE; }

json_t* json_incref(json_t* json) {
	if (json && !shared(json))
		json->refcount++;
	return json;
}
void json_decref(json_t* json) {
	if (json && !shared(json) && --json->refcount == 0)
		destroy(json);
}

// ---- objects -------------------------------------------------------------------------------

size_t json_object_size(const json_t* object) { return json_is_object(object) ? object->members.size() : 0; }
json_t* json_object_get(const json_t* object, const char* key) {
	if (!json_is_object(object) || !key)
		return nullptr;
	Member* m = find(object, key);
	return m ? m->value : nullptr;
}
int json_object_set_new(json_t* object, const char* key, json_t* value) {
	if (!value)
		return -1;
	if (!json_is_object(object) || !key || contains(value, object)) {
		json_decref(value);
		return -1;
	}
	if (Member* m = find(object, key)) {
		json_decref(m->value);
		m->value = value;
		return 0;
	}
	object->members.push_back(Member::make(key, value, object->members.size()));
	return 0;
}
int json_object_set(json_t* object, const char* key, json_t* value) { return json_object_set_new(object, key, json_incref(value)); }
int json_object_set_nocheck(json_t* object, const char* key, json_t* value) { return json_object_set(object, key, value); }
int json_object_set_new_nocheck(json_t* object, const char* key, json_t* value) { return json_object_set_new(object, key, value); }
int json_object_del(json_t* object, const char* key) {
	if (!json_is_object(object) || !key)
		return -1;
	Member* m = find(object, key);
	if (!m)
		return -1;
	object->members.erase(object->members.begin() + m->index);
	for (size_t i = 0; i < object->members.size(); i++)
		object->members[i]->index = i;
	json_decref(m->value);
	std::free(m);
	return 0;
}
int json_object_clear(json_t* object) {
	if (!json_is_object(object))
		return -1;
	for (Member* m : object->members) {
		json_decref(m->value);
		std::free(m);
	}
	object->members.clear();
	return 0;
}
int json_object_update(json_t* object, json_t* other) {
	if (!json_is_object(object) || !json_is_object(other))
		return -1;
	for (Member* m : other->members)
		if (json_object_set(object, m->key(), m->value))
			return -1;
	return 0;
}
void* json_object_iter(json_t* object) {
	if (!json_is_object(object) || object->members.empty())
		return nullptr;
	return object->members[0];
}
void* json_object_iter_at(json_t* object, const char* key) {
	if (!json_is_object(object) || !key)
		return nullptr;
	return find(object, key);
}
void* json_object_key_to_iter(const char* key) { return key ? Member::of(key) : nullptr; }
void* json_object_iter_next(json_t* object, void* iter) {
	if (!json_is_object(object) || !iter)
		return nullptr;
	size_t next = static_cast<Member*>(iter)->index + 1;
	return next < object->members.size() ? object->members[next] : nullptr;
}
const char* json_object_iter_key(void* iter) { return iter ? static_cast<Member*>(iter)->key() : nullptr; }
json_t* json_object_iter_value(void* iter) { return iter ? static_cast<Member*>(iter)->value : nullptr; }
int json_object_iter_set_new(json_t* object, void* iter, json_t* value) {
	if (!json_is_object(object) || !iter || !value) {
		json_decref(value);
		return -1;
	}
	Member* m = static_cast<Member*>(iter);
	json_decref(m->value);
	m->value = value;
	return 0;
}

// ---- arrays --------------------------------------------------------------------------------

size_t json_array_size(const json_t* array) { return json_is_array(array) ? array->items.size() : 0; }
json_t* json_array_get(const json_t* array, size_t index) {
	if (!json_is_array(array) || index >= array->items.size())
		return nullptr;
	return array->items[index];
}
int json_array_set_new(json_t* array, size_t index, json_t* value) {
	if (!value)
		return -1;
	if (!json_is_array(array) || index >= array->items.size() || contains(value, array)) {
		json_decref(value);
		return -1;
	}
	json_decref(array->items[index]);
	array->items[index] = value;
	return 0;
}
int json_array_set(json_t* array, size_t index, json_t* value) { return json_array_set_new(array, index, json_incref(value)); }
int json_array_append_new(json_t* array, json_t* value) {
	if (!value)
		return -1;
	if (!json_is_array(array) || contains(value, array)) {
		json_decref(value);
		return -1;
	}
	array->items.push_back(value);
	return 0;
}
int json_array_append(json_t* array, json_t* value) { return json_array_append_new(array, json_incref(value)); }
int json_array_insert_new(json_t* array, size_t index, json_t* value) {
	if (!value)
		return -1;
	if (!json_is_array(array) || index > array->items.size() || contains(value, array)) {
		json_decref(value);
		return -1;
	}
	array->items.insert(array->items.begin() + index, value);
	return 0;
}
int json_array_insert(json_t* array, size_t index, json_t* value) { return json_array_insert_new(array, index, json_incref(value)); }
int json_array_remove(json_t* array, size_t index) {
	if (!json_is_array(array) || index >= array->items.size())
		return -1;
	json_decref(array->items[index]);
	array->items.erase(array->items.begin() + index);
	return 0;
}
int json_array_clear(json_t* array) {
	if (!json_is_array(array))
		return -1;
	for (json_t* item : array->items)
		json_decref(item);
	array->items.clear();
	return 0;
}
int json_array_extend(json_t* array, json_t* other) {
	if (!json_is_array(array) || !json_is_array(other))
		return -1;
	std::vector<json_t*> more = other->items;
	for (json_t* item : more)
		array->items.push_back(json_incref(item));
	return 0;
}

// ---- values --------------------------------------------------------------------------------

const char* json_string_value(const json_t* string) { return json_is_string(string) ? string->text.c_str() : nullptr; }
size_t json_string_length(const json_t* string) { return json_is_string(string) ? string->text.size() : 0; }
json_int_t json_integer_value(const json_t* integer) { return json_is_integer(integer) ? integer->integer : 0; }
double json_real_value(const json_t* real) { return json_is_real(real) ? real->real : 0.0; }
double json_number_value(const json_t* json) {
	if (json_is_integer(json))
		return (double) json->integer;
	if (json_is_real(json))
		return json->real;
	return 0.0;
}
int json_string_setn(json_t* string, const char* value, size_t len) {
	if (!json_is_string(string) || !value)
		return -1;
	string->text.assign(value, len);
	return 0;
}
int json_string_set(json_t* string, const char* value) { return value ? json_string_setn(string, value, std::strlen(value)) : -1; }
int json_integer_set(json_t* integer, json_int_t value) {
	if (!json_is_integer(integer))
		return -1;
	integer->integer = value;
	return 0;
}
int json_real_set(json_t* real, double value) {
	if (!json_is_real(real) || std::isnan(value) || std::isinf(value))
		return -1;
	real->real = value;
	return 0;
}

int json_equal(const json_t* a, const json_t* b) {
	if (!a || !b)
		return 0;
	if (a == b)
		return 1;
	if (a->type != b->type)
		return 0;
	switch (a->type) {
		case JSON_OBJECT:
			if (a->members.size() != b->members.size())
				return 0;
			for (Member* m : a->members) {
				Member* other = find(b, m->key());
				if (!other || !json_equal(m->value, other->value))
					return 0;
			}
			return 1;
		case JSON_ARRAY:
			if (a->items.size() != b->items.size())
				return 0;
			for (size_t i = 0; i < a->items.size(); i++)
				if (!json_equal(a->items[i], b->items[i]))
					return 0;
			return 1;
		case JSON_STRING: return a->text == b->text;
		case JSON_INTEGER: return a->integer == b->integer;
		case JSON_REAL: return a->real == b->real;
		default: return 1;
	}
}

static json_t* copy(const json_t* value, bool deep) {
	if (!value)
		return nullptr;
	switch (value->type) {
		case JSON_OBJECT: {
			json_t* out = json_object();
			for (Member* m : value->members)
				json_object_set_new(out, m->key(), deep ? copy(m->value, true) : json_incref(m->value));
			return out;
		}
		case JSON_ARRAY: {
			json_t* out = json_array();
			for (json_t* item : value->items)
				json_array_append_new(out, deep ? copy(item, true) : json_incref(item));
			return out;
		}
		case JSON_STRING: return json_stringn(value->text.data(), value->text.size());
		case JSON_INTEGER: return json_integer(value->integer);
		case JSON_REAL: return json_real(value->real);
		default: return const_cast<json_t*>(value);
	}
}
json_t* json_copy(json_t* value) { return copy(value, false); }
json_t* json_deep_copy(const json_t* value) { return copy(value, true); }

} // extern "C"

// ---- reading -------------------------------------------------------------------------------
namespace {

struct Reader {
	const char* at;
	const char* end;
	const char* start;
	size_t flags;
	std::string problem;
	int depth = 0;

	bool fail(const char* what) {
		if (problem.empty())
			problem = what;
		return false;
	}
	void space() {
		while (at < end && (*at == ' ' || *at == '\t' || *at == '\n' || *at == '\r'))
			at++;
	}
	bool literal(const char* word) {
		size_t len = std::strlen(word);
		if ((size_t) (end - at) < len || std::memcmp(at, word, len) != 0)
			return false;
		at += len;
		return true;
	}
	static void utf8(std::string& out, unsigned code) {
		if (code < 0x80)
			out += (char) code;
		else if (code < 0x800) {
			out += (char) (0xc0 | (code >> 6));
			out += (char) (0x80 | (code & 0x3f));
		}
		else if (code < 0x10000) {
			out += (char) (0xe0 | (code >> 12));
			out += (char) (0x80 | ((code >> 6) & 0x3f));
			out += (char) (0x80 | (code & 0x3f));
		}
		else {
			out += (char) (0xf0 | (code >> 18));
			out += (char) (0x80 | ((code >> 12) & 0x3f));
			out += (char) (0x80 | ((code >> 6) & 0x3f));
			out += (char) (0x80 | (code & 0x3f));
		}
	}
	bool hex4(unsigned& code) {
		if (end - at < 4)
			return false;
		code = 0;
		for (int i = 0; i < 4; i++) {
			char c = *at++;
			code <<= 4;
			if (c >= '0' && c <= '9')
				code |= c - '0';
			else if (c >= 'a' && c <= 'f')
				code |= c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				code |= c - 'A' + 10;
			else
				return false;
		}
		return true;
	}
	bool string(std::string& out) {
		if (at >= end || *at != '"')
			return fail("a string is expected");
		at++;
		while (at < end && *at != '"') {
			unsigned char c = (unsigned char) *at++;
			if (c < 0x20)
				return fail("a control character in a string");
			if (c != '\\') {
				out += (char) c;
				continue;
			}
			if (at >= end)
				return fail("a string isn't closed");
			char e = *at++;
			switch (e) {
				case '"': out += '"'; break;
				case '\\': out += '\\'; break;
				case '/': out += '/'; break;
				case 'b': out += '\b'; break;
				case 'f': out += '\f'; break;
				case 'n': out += '\n'; break;
				case 'r': out += '\r'; break;
				case 't': out += '\t'; break;
				case 'u': {
					unsigned code;
					if (!hex4(code))
						return fail("a \\u escape isn't four hex digits");
					if (code >= 0xd800 && code < 0xdc00 && end - at >= 6 && at[0] == '\\' && at[1] == 'u') {
						at += 2;
						unsigned low;
						if (!hex4(low))
							return fail("a \\u escape isn't four hex digits");
						code = 0x10000 + ((code - 0xd800) << 10) + (low - 0xdc00);
					}
					if (code == 0 && !(flags & JSON_ALLOW_NUL))
						return fail("a NUL in a string");
					utf8(out, code);
					break;
				}
				default: return fail("an escape that isn't one");
			}
		}
		if (at >= end)
			return fail("a string isn't closed");
		at++;
		return true;
	}
	json_t* number() {
		const char* from = at;
		bool real = false;
		if (at < end && *at == '-')
			at++;
		while (at < end && *at >= '0' && *at <= '9')
			at++;
		if (at < end && *at == '.') {
			real = true;
			at++;
			while (at < end && *at >= '0' && *at <= '9')
				at++;
		}
		if (at < end && (*at == 'e' || *at == 'E')) {
			real = true;
			at++;
			if (at < end && (*at == '+' || *at == '-'))
				at++;
			while (at < end && *at >= '0' && *at <= '9')
				at++;
		}
		std::string text(from, at);
		if (text.empty() || text == "-") {
			fail("a value is expected");
			return nullptr;
		}
		if (real || (flags & JSON_DECODE_INT_AS_REAL)) {
			double value = std::strtod(text.c_str(), nullptr);
			if (std::isinf(value) || std::isnan(value)) {
				fail("a number too large");
				return nullptr;
			}
			return json_real(value);
		}
		return json_integer(std::strtoll(text.c_str(), nullptr, 10));
	}
	json_t* value() {
		space();
		if (at >= end) {
			fail("it ends where a value is expected");
			return nullptr;
		}
		if (++depth > 512) {
			fail("it's nested too deep");
			return nullptr;
		}
		json_t* out = nullptr;
		char c = *at;
		if (c == '{') {
			at++;
			out = json_object();
			space();
			if (at < end && *at == '}') {
				at++;
			}
			else {
				while (true) {
					space();
					std::string key;
					if (!string(key))
						break;
					space();
					if (at >= end || *at != ':') {
						fail("a ':' is expected after a key");
						break;
					}
					at++;
					json_t* member = value();
					if (!member)
						break;
					if ((flags & JSON_REJECT_DUPLICATES) && json_object_get(out, key.c_str())) {
						json_decref(member);
						fail("a key comes twice");
						break;
					}
					json_object_set_new(out, key.c_str(), member);
					space();
					if (at < end && *at == ',') {
						at++;
						continue;
					}
					if (at < end && *at == '}') {
						at++;
						break;
					}
					fail("a ',' or '}' is expected");
					break;
				}
			}
		}
		else if (c == '[') {
			at++;
			out = json_array();
			space();
			if (at < end && *at == ']') {
				at++;
			}
			else {
				while (true) {
					json_t* item = value();
					if (!item)
						break;
					json_array_append_new(out, item);
					space();
					if (at < end && *at == ',') {
						at++;
						continue;
					}
					if (at < end && *at == ']') {
						at++;
						break;
					}
					fail("a ',' or ']' is expected");
					break;
				}
			}
		}
		else if (c == '"') {
			std::string text;
			if (string(text))
				out = json_stringn(text.data(), text.size());
		}
		else if (literal("true"))
			out = json_true();
		else if (literal("false"))
			out = json_false();
		else if (literal("null"))
			out = json_null();
		else
			out = number();
		depth--;
		if (!problem.empty()) {
			json_decref(out);
			return nullptr;
		}
		return out;
	}
};

void report(json_error_t* error, const Reader& reader, const char* source) {
	if (!error)
		return;
	std::memset(error, 0, sizeof(*error));
	int line = 1, column = 0;
	for (const char* p = reader.start; p < reader.at && p < reader.end; p++) {
		if (*p == '\n') {
			line++;
			column = 0;
		}
		else
			column++;
	}
	error->line = line;
	error->column = column;
	error->position = (int) (reader.at - reader.start);
	std::snprintf(error->source, sizeof(error->source), "%s", source);
	std::snprintf(error->text, sizeof(error->text), "%s", reader.problem.c_str());
}

json_t* read(const char* buffer, size_t length, size_t flags, json_error_t* error, const char* source) {
	if (error)
		std::memset(error, 0, sizeof(*error));
	Reader reader{buffer, buffer + length, buffer, flags, std::string()};
	// (a byte order mark isn't JSON, and files have one)
	if (length >= 3 && std::memcmp(buffer, "\xef\xbb\xbf", 3) == 0)
		reader.at += 3;
	json_t* out = reader.value();
	if (out && !(flags & JSON_DECODE_ANY) && !json_is_object(out) && !json_is_array(out)) {
		json_decref(out);
		out = nullptr;
		reader.fail("an object or an array is expected");
	}
	if (out && !(flags & JSON_DISABLE_EOF_CHECK)) {
		reader.space();
		if (reader.at < reader.end) {
			json_decref(out);
			out = nullptr;
			reader.fail("there's more after the value");
		}
	}
	if (!out)
		report(error, reader, source);
	return out;
}

// ---- writing -------------------------------------------------------------------------------

void quote(std::string& out, const std::string& text, size_t flags) {
	out += '"';
	for (size_t i = 0; i < text.size(); i++) {
		unsigned char c = (unsigned char) text[i];
		switch (c) {
			case '"': out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			case '\b': out += "\\b"; break;
			case '\f': out += "\\f"; break;
			case '\n': out += "\\n"; break;
			case '\r': out += "\\r"; break;
			case '\t': out += "\\t"; break;
			case '/':
				out += (flags & JSON_ESCAPE_SLASH) ? "\\/" : "/";
				break;
			default:
				if (c < 0x20 || c == 0x7f) {
					char buf[8];
					std::snprintf(buf, sizeof(buf), "\\u%04x", c);
					out += buf;
				}
				else if (c >= 0x80 && (flags & JSON_ENSURE_ASCII)) {
					// one character of UTF-8, as \u escapes
					int more = (c >= 0xf0) ? 3 : (c >= 0xe0) ? 2 : (c >= 0xc0) ? 1 : 0;
					unsigned code = (more == 3) ? (c & 0x07) : (more == 2) ? (c & 0x0f) : (more == 1) ? (c & 0x1f) : c;
					for (int k = 0; k < more && i + 1 < text.size(); k++)
						code = (code << 6) | ((unsigned char) text[++i] & 0x3f);
					char buf[16];
					if (code >= 0x10000) {
						code -= 0x10000;
						std::snprintf(buf, sizeof(buf), "\\u%04x\\u%04x", 0xd800 + (code >> 10), 0xdc00 + (code & 0x3ff));
					}
					else
						std::snprintf(buf, sizeof(buf), "\\u%04x", code);
					out += buf;
				}
				else
					out += (char) c;
		}
	}
	out += '"';
}

void write(std::string& out, const json_t* json, size_t flags, int level) {
	int indent = (int) (flags & JSON_MAX_INDENT);
	auto newline = [&](int level) {
		if (indent > 0) {
			out += '\n';
			out.append((size_t) level * indent, ' ');
		}
		else if (!(flags & JSON_COMPACT))
			out += ' ';
	};
	switch (json->type) {
		case JSON_NULL: out += "null"; break;
		case JSON_TRUE: out += "true"; break;
		case JSON_FALSE: out += "false"; break;
		case JSON_INTEGER: out += std::to_string(json->integer); break;
		case JSON_REAL: {
			int precision = (int) ((flags >> 11) & 0x1f);
			char buf[64];
			std::snprintf(buf, sizeof(buf), "%.*g", precision ? precision : 17, json->real);
			// (a real stays a real when it's read again)
			if (!std::strpbrk(buf, ".eE"))
				std::strcat(buf, ".0");
			out += buf;
			break;
		}
		case JSON_STRING: quote(out, json->text, flags); break;
		case JSON_ARRAY: {
			out += '[';
			if (json->items.empty()) {
				out += ']';
				break;
			}
			for (size_t i = 0; i < json->items.size(); i++) {
				if (i > 0)
					out += ',';
				if (indent > 0 || i > 0)
					newline(level + 1);
				write(out, json->items[i], flags, level + 1);
			}
			if (indent > 0)
				newline(level);
			out += ']';
			break;
		}
		case JSON_OBJECT: {
			out += '{';
			if (json->members.empty()) {
				out += '}';
				break;
			}
			std::vector<Member*> members = json->members;
			if (flags & JSON_SORT_KEYS)
				std::sort(members.begin(), members.end(), [](Member* a, Member* b) { return std::strcmp(a->key(), b->key()) < 0; });
			for (size_t i = 0; i < members.size(); i++) {
				if (i > 0)
					out += ',';
				if (indent > 0 || i > 0)
					newline(level + 1);
				quote(out, members[i]->key(), flags);
				out += (flags & JSON_COMPACT) ? ":" : ": ";
				write(out, members[i]->value, flags, level + 1);
			}
			if (indent > 0)
				newline(level);
			out += '}';
			break;
		}
	}
}

} // namespace

extern "C" {

json_t* json_loadb(const char* buffer, size_t buflen, size_t flags, json_error_t* error) {
	if (!buffer)
		return nullptr;
	return read(buffer, buflen, flags, error, "<buffer>");
}
json_t* json_loads(const char* input, size_t flags, json_error_t* error) {
	if (!input)
		return nullptr;
	return read(input, std::strlen(input), flags, error, "<string>");
}
json_t* json_loadf(FILE* input, size_t flags, json_error_t* error) {
	if (!input)
		return nullptr;
	std::string text;
	char chunk[4096];
	size_t got;
	while ((got = std::fread(chunk, 1, sizeof(chunk), input)) > 0)
		text.append(chunk, got);
	return read(text.data(), text.size(), flags, error, "<stream>");
}
json_t* json_load_file(const char* path, size_t flags, json_error_t* error) {
	if (error)
		std::memset(error, 0, sizeof(*error));
	FILE* f = path ? std::fopen(path, "rb") : nullptr;
	if (!f) {
		if (error) {
			error->line = -1;
			std::snprintf(error->source, sizeof(error->source), "%s", path ? path : "");
			std::snprintf(error->text, sizeof(error->text), "unable to open %s", path ? path : "(no file)");
		}
		return nullptr;
	}
	json_t* out = json_loadf(f, flags, error);
	std::fclose(f);
	if (!out && error)
		std::snprintf(error->source, sizeof(error->source), "%s", path);
	return out;
}

char* json_dumps(const json_t* json, size_t flags) {
	if (!json)
		return nullptr;
	if (!(flags & JSON_ENCODE_ANY) && !json_is_object(json) && !json_is_array(json))
		return nullptr;
	std::string out;
	write(out, json, flags, 0);
	char* text = static_cast<char*>(std::malloc(out.size() + 1));
	std::memcpy(text, out.c_str(), out.size() + 1);
	return text;
}
int json_dumpf(const json_t* json, FILE* output, size_t flags) {
	char* text = json_dumps(json, flags);
	if (!text || !output) {
		std::free(text);
		return -1;
	}
	size_t len = std::strlen(text);
	bool ok = std::fwrite(text, 1, len, output) == len;
	std::free(text);
	return ok ? 0 : -1;
}
int json_dump_file(const json_t* json, const char* path, size_t flags) {
	FILE* f = path ? std::fopen(path, "wb") : nullptr;
	if (!f)
		return -1;
	int result = json_dumpf(json, f, flags);
	if (std::fclose(f) != 0)
		return -1;
	return result;
}

} // extern "C"

// ---- json_pack and json_unpack: values by a format ------------------------------------------
namespace {

/// A format being read, with its arguments: `s` a string, `n` null, `b` a
/// bool (int), `i` an int, `I` a json_int_t, `f` a real (`F` unpacking any
/// number), `o` a value (taken; `O` a reference added), `[…]` an array,
/// `{s:…}` an object. `?` after a value packs a null for a null pointer
/// (`*` leaves it out); unpacking, `?` after a key lets it be missing, and
/// `!` before a close says nothing may be left unread (`*`: anything may).
struct Format {
	const char* at;
	va_list ap;
	size_t flags;
	std::string problem;

	char next() {
		while (*at && (std::isspace((unsigned char) *at) || *at == ',' || *at == ':'))
			at++;
		return *at;
	}
	bool fail(const std::string& why) {
		if (problem.empty())
			problem = why;
		return false;
	}
};

void formatError(json_error_t* error, const Format& f) {
	if (!error)
		return;
	std::memset(error, 0, sizeof(*error));
	error->line = -1;
	error->column = -1;
	std::snprintf(error->source, sizeof(error->source), "%s", "<format>");
	std::snprintf(error->text, sizeof(error->text), "%s", f.problem.c_str());
}

/// A string spec's string: `s`, `s#` (int length), `s%` (size_t length),
/// joined by `+`. `isNull` where a pointer is null.
void packString(Format& f, std::string& out, bool& isNull) {
	isNull = false;
	while (true) {
		const char* s = va_arg(f.ap, const char*);
		size_t len = std::string::npos;
		if (*f.at == '#') {
			len = (size_t) va_arg(f.ap, int);
			f.at++;
		}
		else if (*f.at == '%') {
			len = va_arg(f.ap, size_t);
			f.at++;
		}
		if (!s)
			isNull = true;
		else
			out.append(s, len == std::string::npos ? std::strlen(s) : len);
		if (*f.at != '+')
			return;
		f.at++;
	}
}

/// The next value of the format, or null (with `skip` set where `*` says
/// to leave a null one out).
json_t* pack(Format& f, bool& skip) {
	skip = false;
	char c = f.next();
	if (!c) {
		f.fail("the format ends early");
		return nullptr;
	}
	f.at++;
	// (a nullable value: `?` gives null for a null pointer, `*` nothing)
	auto nullable = [&](json_t* v, bool wasNull) -> json_t* {
		if (*f.at == '?' || *f.at == '*') {
			bool omit = *f.at == '*';
			f.at++;
			if (wasNull) {
				skip = omit;
				return omit ? nullptr : json_null();
			}
		}
		else if (wasNull) {
			f.fail("a null where a value was needed");
		}
		return v;
	};
	switch (c) {
		case 'n':
			return json_null();
		case 'b':
			return json_boolean(va_arg(f.ap, int));
		case 'i':
			return json_integer(va_arg(f.ap, int));
		case 'I':
			return json_integer(va_arg(f.ap, json_int_t));
		case 'f':
			return json_real(va_arg(f.ap, double));
		case 's': {
			std::string s;
			bool isNull;
			packString(f, s, isNull);
			return nullable(isNull ? nullptr : json_stringn(s.c_str(), s.size()), isNull);
		}
		case 'o':
		case 'O': {
			json_t* v = va_arg(f.ap, json_t*);
			if (v && c == 'O')
				json_incref(v);
			return nullable(v, !v);
		}
		case '[': {
			json_t* array = json_array();
			while (f.next() != ']') {
				bool leave;
				json_t* v = pack(f, leave);
				if (!v && !leave) {
					json_decref(array);
					return nullptr;
				}
				if (v)
					json_array_append_new(array, v);
			}
			f.at++;
			return array;
		}
		case '{': {
			json_t* object = json_object();
			while (f.next() != '}') {
				if (f.next() != 's') {
					f.fail("an object's key must be a string (s)");
					json_decref(object);
					return nullptr;
				}
				f.at++;
				std::string key;
				bool isNull, leave;
				packString(f, key, isNull);
				json_t* v = pack(f, leave);
				if (isNull || (!v && !leave)) {
					if (isNull)
						f.fail("a null key");
					json_decref(v);
					json_decref(object);
					return nullptr;
				}
				if (v)
					json_object_set_new(object, key.c_str(), v);
			}
			f.at++;
			return object;
		}
		default:
			f.fail(std::string("unknown format character '") + c + "'");
			return nullptr;
	}
}

/// Reads `json` by the next value of the format into its arguments (none
/// with `JSON_VALIDATE_ONLY`). `store`: false for a key that's missing, so
/// its arguments are passed over.
bool unpack(Format& f, json_t* json, bool store) {
	char c = f.next();
	if (!c)
		return f.fail("the format ends early");
	f.at++;
	bool only = f.flags & JSON_VALIDATE_ONLY;
	auto want = [&](bool ok, const char* what) {
		return ok || !store || f.fail(std::string("expected ") + what);
	};
	switch (c) {
		case 'n':
			return want(json_is_null(json), "null");
		case 'b': {
			int* p = only ? nullptr : va_arg(f.ap, int*);
			if (!want(json_is_boolean(json), "true or false"))
				return false;
			if (p && store)
				*p = json_is_true(json);
			return true;
		}
		case 'i': {
			int* p = only ? nullptr : va_arg(f.ap, int*);
			if (!want(json_is_integer(json), "an integer"))
				return false;
			if (p && store)
				*p = (int) json_integer_value(json);
			return true;
		}
		case 'I': {
			json_int_t* p = only ? nullptr : va_arg(f.ap, json_int_t*);
			if (!want(json_is_integer(json), "an integer"))
				return false;
			if (p && store)
				*p = json_integer_value(json);
			return true;
		}
		case 'f':
		case 'F': {
			double* p = only ? nullptr : va_arg(f.ap, double*);
			if (!want(c == 'f' ? json_is_real(json) : json_is_number(json), c == 'f' ? "a real" : "a number"))
				return false;
			if (p && store)
				*p = json_number_value(json);
			return true;
		}
		case 's': {
			const char** p = only ? nullptr : va_arg(f.ap, const char**);
			size_t* len = nullptr;
			if (*f.at == '%') {
				f.at++;
				len = only ? nullptr : va_arg(f.ap, size_t*);
			}
			if (!want(json_is_string(json), "a string"))
				return false;
			if (p && store)
				*p = json_string_value(json);
			if (len && store)
				*len = json_string_length(json);
			return true;
		}
		case 'o':
		case 'O': {
			json_t** p = only ? nullptr : va_arg(f.ap, json_t**);
			if (!want(json != nullptr, "a value"))
				return false;
			if (p && store)
				*p = c == 'O' ? json_incref(json) : json;
			return true;
		}
		case '[': {
			if (!want(json_is_array(json), "an array"))
				return false;
			size_t i = 0;
			bool strict = f.flags & JSON_STRICT;
			while (true) {
				char d = f.next();
				if (d == ']' || d == '!' || d == '*') {
					f.at++;
					if (d != ']') {
						strict = d == '!';
						if (f.next() != ']')
							return f.fail("expected ] after ! or *");
						f.at++;
					}
					break;
				}
				json_t* item = store ? json_array_get(json, i) : nullptr;
				if (store && !item)
					return f.fail("the array is too short");
				if (!unpack(f, item, store))
					return false;
				i++;
			}
			if (store && strict && i < json_array_size(json))
				return f.fail("the array has more than the format reads");
			return true;
		}
		case '{': {
			if (!want(json_is_object(json), "an object"))
				return false;
			size_t read = 0;
			bool strict = f.flags & JSON_STRICT;
			while (true) {
				char d = f.next();
				if (d == '}' || d == '!' || d == '*') {
					f.at++;
					if (d != '}') {
						strict = d == '!';
						if (f.next() != '}')
							return f.fail("expected } after ! or *");
						f.at++;
					}
					break;
				}
				if (d != 's')
					return f.fail("an object's key must be a string (s)");
				f.at++;
				const char* key = va_arg(f.ap, const char*);
				bool optional = *f.at == '?';
				if (optional)
					f.at++;
				json_t* value = store && key ? json_object_get(json, key) : nullptr;
				if (store && !value && !optional)
					return f.fail(std::string("the object has no ") + (key ? key : "(null)"));
				if (value)
					read++;
				if (!unpack(f, value, store && value))
					return false;
			}
			if (store && strict && read < json_object_size(json))
				return f.fail("the object has keys the format doesn't read");
			return true;
		}
		default:
			return f.fail(std::string("unknown format character '") + c + "'");
	}
}

} // namespace

extern "C" {
json_t* json_vpack_ex(json_error_t* error, size_t flags, const char* fmt, va_list ap) {
	Format f;
	f.at = fmt ? fmt : "";
	f.flags = flags;
	va_copy(f.ap, ap);
	bool skip;
	json_t* value = pack(f, skip);
	if (value && f.next())
		f.fail("the format goes on after its value");
	va_end(f.ap);
	if (!f.problem.empty()) {
		json_decref(value);
		formatError(error, f);
		return nullptr;
	}
	return value;
}
json_t* json_pack_ex(json_error_t* error, size_t flags, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	json_t* value = json_vpack_ex(error, flags, fmt, ap);
	va_end(ap);
	return value;
}
json_t* json_pack(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	json_t* value = json_vpack_ex(nullptr, 0, fmt, ap);
	va_end(ap);
	return value;
}
int json_vunpack_ex(json_t* root, json_error_t* error, size_t flags, const char* fmt, va_list ap) {
	Format f;
	f.at = fmt ? fmt : "";
	f.flags = flags;
	va_copy(f.ap, ap);
	bool ok = unpack(f, root, true);
	if (ok && f.next())
		ok = f.fail("the format goes on after its value");
	va_end(f.ap);
	if (!ok)
		formatError(error, f);
	return ok ? 0 : -1;
}
int json_unpack_ex(json_t* root, json_error_t* error, size_t flags, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int result = json_vunpack_ex(root, error, flags, fmt, ap);
	va_end(ap);
	return result;
}
int json_unpack(json_t* root, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int result = json_vunpack_ex(root, nullptr, 0, fmt, ap);
	va_end(ap);
	return result;
}
} // extern "C"
