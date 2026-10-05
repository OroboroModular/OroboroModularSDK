/*
 * The file and message dialogs Rack modules open from their menus
 * (osdialog's interface), opened as the system's own (rack/src/osdialog.cpp:
 * on Windows; elsewhere they answer as a dialog that was closed, for now).
 * The Oroboro Modular SDK's own header.
 */
#ifndef OROBORO_RACK_OSDIALOG_H
#define OROBORO_RACK_OSDIALOG_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { OSDIALOG_INFO, OSDIALOG_WARNING, OSDIALOG_ERROR } osdialog_message_level;
typedef enum { OSDIALOG_OK, OSDIALOG_OK_CANCEL, OSDIALOG_YES_NO } osdialog_message_buttons;
typedef enum { OSDIALOG_OPEN, OSDIALOG_OPEN_DIR, OSDIALOG_SAVE } osdialog_file_action;

typedef struct osdialog_filter_patterns {
	char *pattern;
	struct osdialog_filter_patterns *next;
} osdialog_filter_patterns;

typedef struct osdialog_filters {
	char *name;
	osdialog_filter_patterns *patterns;
	struct osdialog_filters *next;
} osdialog_filters;

typedef struct {
	uint8_t r, g, b, a;
} osdialog_color;

int osdialog_message(osdialog_message_level level, osdialog_message_buttons buttons, const char *message);
char *osdialog_prompt(osdialog_message_level level, const char *message, const char *text);
char *osdialog_file(osdialog_file_action action, const char *dir, const char *filename, osdialog_filters *filters);
int osdialog_color_picker(osdialog_color *color, int opacity);
osdialog_filters *osdialog_filters_parse(const char *str);
void osdialog_filters_free(osdialog_filters *filters);
char *osdialog_strdup(const char *s);
char *osdialog_strndup(const char *s, size_t n);

/* While set (the bridge sets it as it gives a module the pointer), the
 * dialogs answer as closed: they're opened from a menu's choice, on a
 * thread of their own, never from where the plugin's window is drawn. */
extern int oroboro_osdialog_closed;

#ifdef __cplusplus
}
#endif

#endif
