#ifndef _CONFIG_H
#define _CONFIG_H

#include "wrap.h"

#define CKEY GDK_CONTROL_MASK
#define SKEY GDK_SHIFT_MASK
#define MKEY GDK_SUPER_MASK
#define AKEY GDK_MOD1_MASK

struct key {
	guint mod;
	guint keyval;
	void (*func)(union arg *);
	union arg arg;
};

static struct key keys[] = {
	{AKEY, GDK_KEY_q, wrap_window_close, {0}},
	{CKEY, GDK_KEY_r, wrap_uri_reload, {0}},
	{CKEY|SKEY, GDK_KEY_r, wrap_uri_reload, {1}},
	{0, GDK_KEY_F5, wrap_uri_reload, {0}},
	{CKEY, GDK_KEY_F5, wrap_uri_reload, {1}},
	{0, GDK_KEY_Escape, wrap_uri_stop, {0}},
	{0, GDK_KEY_F11, wrap_fullscreen_toggle, {0}},
	{AKEY, GDK_KEY_h, wrap_uri_search_engine_load, {0}},
	{CKEY, GDK_KEY_f, wrap_win_find_build, {0}},
	{CKEY, GDK_KEY_n, wrap_find_next, {0}},
	{CKEY|SKEY, GDK_KEY_n, wrap_find_previous, {0}},
	{CKEY, GDK_KEY_b, wrap_bookmark_toggle, {0}},
	{CKEY, GDK_KEY_d, wrap_dark_mode_toggle, {0}},
	{CKEY|SKEY, GDK_KEY_d, wrap_debug_toggle, {0}},
	{CKEY, GDK_KEY_s, wrap_download_page, {0}},
	{AKEY, GDK_KEY_s, wrap_focus_entry, {0}},
	{AKEY|SKEY, GDK_KEY_s, wrap_focus_secondary_entry, {0}},

	{AKEY, GDK_KEY_Left, wrap_navigate, {1}},
	{AKEY, GDK_KEY_Right, wrap_navigate, {0}},

	{CKEY, GDK_KEY_equal, wrap_zoom_action, {1}},
	{CKEY, GDK_KEY_minus, wrap_zoom_action, {0}},
	{CKEY, GDK_KEY_0, wrap_zoom_action, {2}},

	{CKEY, GDK_KEY_Tab, wrap_view_order_show, {1}},
	{CKEY|SKEY, GDK_KEY_ISO_Left_Tab, wrap_view_order_show, {0}},

	{AKEY, GDK_KEY_1, wrap_view_show, {0}},
	{AKEY, GDK_KEY_2, wrap_view_show, {1}},
	{AKEY, GDK_KEY_3, wrap_view_show, {2}},
	{AKEY, GDK_KEY_4, wrap_view_show, {3}},
	{AKEY, GDK_KEY_5, wrap_view_show, {4}},
	{AKEY, GDK_KEY_6, wrap_view_show, {5}},
	{AKEY, GDK_KEY_7, wrap_view_show, {6}},
	{AKEY, GDK_KEY_8, wrap_view_show, {7}},
	{AKEY, GDK_KEY_9, wrap_view_show, {8}},
	{AKEY, GDK_KEY_0, wrap_view_show, {9}},
};

#endif /* _CONFIG_H */
