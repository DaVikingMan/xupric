#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <string.h>

#include "fun/sql/bookmark.h"
#include "fun/sql/history.h"
#include "frame/frame.h"
#include "frame/style/style.h"
#include "util/util.h"
#include "cfg/cfg.h"
#include "cfg/config.h"
#include "uri/uri.h"

#include "download.h"
#include "view.h"

static void *uri_blank_handle(WebKitWebView *, WebKitNavigationAction *na);
static void uri_changed(WebKitWebView *);
static void uri_load_progress(WebKitWebView *v);
static int permission_request(WebKitWebView *, WebKitPermissionRequest *r, GtkWindow *p);

static WebKitWebView **views;
static int last = 0;
static char *uri_last = "";

void view_order_show(int increment)
{
	if (increment) {
		if (last >= 9)
			view_show(0);
		else
			view_show(last+1);
	} else {
		if (last <= 0)
			view_show(9);
		else
			view_show(last-1);
	}
}

void view_show(int id)
{
	GtkBuilder *builder;
	WebKitWebView *dview;
	GtkImage *dark_mode_image;
	GtkWidget *box;
	struct frame *frames;
	char zoom[5];

	frames = frames_get();
	builder = builder_get();
	box = GTK_WIDGET(gtk_builder_get_object(builder, "main_box"));
	dview = g_object_ref(frames[last].view);

	gtk_container_remove(GTK_CONTAINER(box), GTK_WIDGET(frames[last].view));
	gtk_box_pack_end(GTK_BOX(box), GTK_WIDGET(frames[id].view), TRUE, TRUE, 0);

	gtk_widget_show_all(frames[id].win);

	gtk_widget_grab_focus(GTK_WIDGET(frames[id].view));
	frames[last].view = dview;
	last = id;

	if (frames[id].empty) {
		uri_search_engine_load(&frames[id]);
		frames[id].empty = 0;
	}

	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(builder, "bar_uri_entry")),
		uri_get(&frames[id]));
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(builder,
		"bar_uri_entry_secondary")), "");

	sprintf(zoom, "%i%%", (int)(frames[id].zoom*100));
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder,
	"menu_zoom_reset_label")), zoom);

	dark_mode_image = GTK_IMAGE(gtk_builder_get_object(builder, "dark_mode_image"));
	if (frames[id].dark_mode)
		gtk_image_set_from_icon_name(dark_mode_image, "night-light-symbolic", 18);
	else
		gtk_image_set_from_icon_name(dark_mode_image, "night-light-disabled-symbolic",
			18);
}

void view_list_create(void)
{
    WebKitSettings *settings;
    WebKitWebContext *context;
    WebKitCookieManager *cookiemanager;
    WebKitWebsiteDataManager *datamanager;
	WebKitUserStyleSheet **css;
	WebKitUserScript **script;
	GTlsCertificate *cert;
	conf_opt *config;
	char *content, *file;
	int i, j;

	config = cfg_get();
    settings = webkit_settings_new_with_settings(
		"enable-java", config[conf_java].i,
		"enable-javascript", config[conf_javascript].i,
		"enable-xss-auditor", config[conf_xss_auditor].i,
		"javascript-can-open-windows-automatically", config[conf_js_auto_popups].i,
		"default-font-family", config[conf_font_family].s,
		"default-font-size", config[conf_font_size].i,
		"default-charset", config[conf_charset].s,
		"enable-developer-extras", config[conf_developer_extras].i,
		"enable-dns-prefetching", config[conf_dns_prefetching].i,
		"enable-caret-browsing", config[conf_caret_browsing].i,
		"enable-media", config[conf_media].i,
		"enable-webaudio", config[conf_webaudio].i,
		"enable-webgl", config[conf_webgl].i,
		"enable-site-specific-quirks", config[conf_site_quirks].i,
		"enable-smooth-scrolling", config[conf_smooth_scrolling].i,
		NULL
	);

	if (strcmp(config[conf_user_agent].s, ""))
		webkit_settings_set_user_agent(settings, config[conf_user_agent].s);

	if (config[conf_hardware_accel].i == 0)
		webkit_settings_set_hardware_acceleration_policy(settings,
			WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);
	else if (config[conf_hardware_accel].i == 1)
		webkit_settings_set_hardware_acceleration_policy(settings,
			WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS);
	else
		webkit_settings_set_hardware_acceleration_policy(settings,
			WEBKIT_HARDWARE_ACCELERATION_POLICY_ON_DEMAND);

	datamanager = webkit_website_data_manager_new(
		"base-cache-directory", config[conf_cache_prefix],
		"base-data-directory", cache_names[1],
		"itp-directory", cache_names[2],
		"offline-application-cache-directory", cache_names[3],
		"hsts-cache-directory", cache_names[4],
		NULL
	);

	webkit_website_data_manager_set_itp_enabled(datamanager, config[conf_itp].i);
	webkit_website_data_manager_set_tls_errors_policy(datamanager,
		config[conf_tls_error_policy].i);

	if (config[conf_ephemeral].i)
		context = webkit_web_context_new_ephemeral();
	else
		context = webkit_web_context_new_with_website_data_manager(datamanager);

	cookiemanager = webkit_web_context_get_cookie_manager(context);

	webkit_web_context_set_process_model(context,
		WEBKIT_PROCESS_MODEL_MULTIPLE_SECONDARY_PROCESSES);

	webkit_web_context_set_cache_model(context,
		config[conf_ephemeral].i ? WEBKIT_CACHE_MODEL_WEB_BROWSER :
		WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);

	if (!config[conf_ephemeral].i) {
		if (config[conf_cookie_policy].i == 0)
			webkit_cookie_manager_set_accept_policy(cookiemanager,
				WEBKIT_COOKIE_POLICY_ACCEPT_NEVER);
		else if (config[conf_cookie_policy].i == 1)
			webkit_cookie_manager_set_accept_policy(cookiemanager,
				WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS);
		else
			webkit_cookie_manager_set_accept_policy(cookiemanager,
				WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY);

		webkit_cookie_manager_set_persistent_storage(cookiemanager,
			cache_names[0], WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
	}

	views = ecalloc(10, sizeof(WebKitWebView));
	css = ecalloc(style_names_len, sizeof(WebKitUserStyleSheet *));
	script = ecalloc(script_names_len, sizeof(WebKitUserScript *));

	for (i = 0; i < style_names_len; i++) {
		file = g_strdup_printf("%s%s", config_names[2], style_names[i]);
		if (!g_file_get_contents(file, &content, NULL, NULL))
			die(1, "[ERROR] Unable to read style file: %s\n", file);

		const gchar *allow[] = { g_strdup_printf("https://%s/*", style_names[i]), NULL };
		css[i] = webkit_user_style_sheet_new(content,
			WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
			WEBKIT_USER_STYLE_LEVEL_USER,
			allow,
			NULL);

		g_free(content);
		g_free(file);
	}

	for (i = 0; i < script_names_len; i++) {
		file = g_strdup_printf("%s%s", config_names[1], script_names[i]);
		if (!g_file_get_contents(file, &content, NULL, NULL))
			die(1, "[ERROR] Unable to read script file: %s\n", file);

		const gchar *allow[] = { g_strdup_printf("https://%s/*", script_names[i]), NULL };
		script[i] = webkit_user_script_new(content,
			WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
			WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
			allow,
			NULL);

		g_free(content);
		g_free(file);
	}

	for (i = 0; i < cert_names_len; i++) {
		file = g_strdup_printf("%s%s", config_names[3], cert_names[i]);
		cert = g_tls_certificate_new_from_file(file, NULL);

		if (g_tls_certificate_verify(cert, NULL, NULL) & G_TLS_CERTIFICATE_VALIDATE_ALL)
			die(1, "[ERROR] Certificate validation failed\n");

		webkit_web_context_allow_tls_certificate_for_host(context, cert, config_names[3]);

		g_free(file);
	}

	for (i = 0; i < 10; i++) {
		views[i] = g_object_new(WEBKIT_TYPE_WEB_VIEW,
			"settings", settings,
			"user-content-manager", webkit_user_content_manager_new(),
			"web-context", context,
			NULL);

		for (j = 0; j < style_names_len; j++) {
			webkit_user_content_manager_add_style_sheet(
				webkit_web_view_get_user_content_manager(views[i]),
				css[j]);
		}

		for (j = 0; j < script_names_len; j++) {
			webkit_user_content_manager_add_script(
				webkit_web_view_get_user_content_manager(views[i]),
				script[j]);
		}

		if (config[conf_dark_mode].i)
			dark_mode_set(views[i]);
		if (config[conf_scrollbar].i)
			style_file_set(views[i], config_names[5]);

		g_signal_connect(G_OBJECT(views[i]), "permission-request",
			G_CALLBACK(permission_request), current_frame_get()->win);
		g_signal_connect(G_OBJECT(views[i]), "load-changed",
			G_CALLBACK(uri_changed), NULL);
		g_signal_connect(G_OBJECT(views[i]), "notify::estimated-load-progress",
			G_CALLBACK(uri_load_progress), NULL);
		g_signal_connect(G_OBJECT(views[i]), "create",
			G_CALLBACK(uri_blank_handle), NULL);
	}
	g_signal_connect(G_OBJECT(context), "download-started",
		G_CALLBACK(download_started), NULL);

	if (style_names_len > 0)
		efree(css);
	if (script_names_len > 0)
		efree(script);
}

static void *uri_blank_handle(WebKitWebView *, WebKitNavigationAction *na)
{
	uri_custom_load(current_frame_get(), (char *)webkit_uri_request_get_uri(
		webkit_navigation_action_get_request(na)), 0);

	return NULL;
}

static void uri_changed(WebKitWebView *)
{
	GtkBuilder *builder;
	GtkCssProvider *css;
	GtkImage *bookmark_image;
	GtkEntry *e;
	char *uri, *title;

	builder = builder_get();
	e = GTK_ENTRY(gtk_builder_get_object(builder, "bar_uri_entry"));
	uri = uri_get(current_frame_get());
	css = gtk_css_provider_new();
	if (strcmp(gtk_entry_get_text(e), uri)) {
		gtk_entry_set_text(e, uri);
	}

	bookmark_image = GTK_IMAGE(gtk_builder_get_object(builder, "bookmark_image"));
	if (bookmark_exists(uri)) {
		gtk_css_provider_load_from_data(css, "#bookmark_image { color: #f0c674; }",
			-1, NULL);
		gtk_image_set_from_icon_name(bookmark_image, "starred-symbolic", 18);
	} else {
		gtk_css_provider_load_from_data(css, "#bookmark_image { color: #abadac; }",
			-1, NULL);
		gtk_image_set_from_icon_name(bookmark_image, "non-starred-symbolic", 18);
	}
	gtk_style_context_add_provider(gtk_widget_get_style_context(
		GTK_WIDGET(bookmark_image)), GTK_STYLE_PROVIDER(css),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	if (strcmp(uri, uri_last))
		history_add(uri);
	uri_last = uri;

	title = ecalloc(60, sizeof(char));
	strcpy(title, "Xupric <");
	strncat(title, uri, 50);
	strcat(title, ">");
	gtk_window_set_title(GTK_WINDOW(current_frame_get()->win), title);
	efree(title);
}

static void uri_load_progress(WebKitWebView *v)
{
	GdkCursor *c;

	if (webkit_web_view_get_estimated_load_progress(v) != 1.0)
		c = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_WATCH);
	else
		c = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_ARROW);
	gdk_window_set_cursor(gtk_widget_get_window(current_frame_get()->win), c);
}

static int permission_request(WebKitWebView *, WebKitPermissionRequest *r, GtkWindow *p)
{
	GtkWidget *dialog;
	conf_opt *config;
	char *type, *question;
	int ret;

	config = cfg_get();
	if (WEBKIT_IS_DEVICE_INFO_PERMISSION_REQUEST(r)) {
		webkit_permission_request_allow(r);
		return 1;
	} else if (WEBKIT_IS_GEOLOCATION_PERMISSION_REQUEST(r)) {
		if (config[conf_permission_geolocation].i == 1) {
			webkit_permission_request_allow(r);
			return 1;
		} else if (config[conf_permission_geolocation].i == 0) {
			webkit_permission_request_deny(r);
			return 1;
		}
		type = "geolocation";
	} else if (WEBKIT_IS_INSTALL_MISSING_MEDIA_PLUGINS_PERMISSION_REQUEST(r)) {
		webkit_permission_request_allow(r);
		return 1;
	} else if (WEBKIT_IS_MEDIA_KEY_SYSTEM_PERMISSION_REQUEST(r)) {
		webkit_permission_request_deny(r);
		return 1;
	} else if (WEBKIT_IS_NOTIFICATION_PERMISSION_REQUEST(r)) {
		if (config[conf_permission_notification].i == 1) {
			webkit_permission_request_allow(r);
			return 1;
		} else if (config[conf_permission_notification].i == 0) {
			webkit_permission_request_deny(r);
			return 1;
		}
		type = "notification";
	} else if (WEBKIT_IS_POINTER_LOCK_PERMISSION_REQUEST(r)) {
		webkit_permission_request_deny(r);
		return 1;
	} else if (WEBKIT_IS_USER_MEDIA_PERMISSION_REQUEST(r)) {
		if (webkit_user_media_permission_is_for_audio_device(
			WEBKIT_USER_MEDIA_PERMISSION_REQUEST(r))) {
			if (config[conf_permission_microphone].i == 1) {
				webkit_permission_request_allow(r);
				return 1;
			} else if (config[conf_permission_microphone].i == 0) {
				webkit_permission_request_deny(r);
				return 1;
			}
			type = "microphone";
		} else if (webkit_user_media_permission_is_for_video_device(
			WEBKIT_USER_MEDIA_PERMISSION_REQUEST(r))) {
			if (config[conf_permission_camera].i == 1) {
				webkit_permission_request_allow(r);
				return 1;
			} else if (config[conf_permission_camera].i == 0) {
				webkit_permission_request_deny(r);
				return 1;
			}
			type = "camera";
		} else {
			type = "media";
		}
	} else if (WEBKIT_IS_WEBSITE_DATA_ACCESS_PERMISSION_REQUEST(r)) {
		webkit_permission_request_deny(r);
		return 1;
	} else {
		return 0;
	}

	question = ecalloc(30+strlen(uri_get(current_frame_get())), sizeof(char));
	strcpy(question, "Allow ");
	strcat(question, type);
	strcat(question, " access?\n");
	strcat(question, uri_get(current_frame_get()));

	dialog = gtk_message_dialog_new(p, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
		GTK_BUTTONS_YES_NO, question);

	gtk_widget_show(dialog);
	ret = gtk_dialog_run(GTK_DIALOG(dialog));

	switch (ret) {
	case GTK_RESPONSE_YES:
		webkit_permission_request_allow(r);
		break;
	default:
		webkit_permission_request_deny(r);
		break;
	}

	gtk_widget_destroy(dialog);
	efree(question);
	return 1;
}

WebKitWebView **views_get(void)
{
	return views;
}

int view_last_get(void)
{
	return last;
}

void view_cleanup(void)
{
	free(views);
}
