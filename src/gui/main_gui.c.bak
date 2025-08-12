// src/gui/main_gui.c
//
// PAC-RF GUI (GTK4) — Future-proof, table-driven output handling
// Layout: Big, readable panes via GtkPaned (left: Terminal over Logs; right: Image)

#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>

#include "interface.h"
#include "logger.h"

// ---- Compatibility guard --------------------------------------------------
// Some distros (older GLib/GIO) don't define G_APPLICATION_DEFAULT_FLAGS.
// Map it to G_APPLICATION_FLAGS_NONE so the code builds everywhere.
#ifndef G_APPLICATION_DEFAULT_FLAGS
#define G_APPLICATION_DEFAULT_FLAGS G_APPLICATION_FLAGS_NONE
#endif

// ==============================
// Application state
// ==============================
typedef struct {
    GtkApplication *gtk_app;
    GtkWidget     *window;

    // Controls
    GtkWidget     *btn_gps;
    GtkWidget     *btn_capture;
    GtkWidget     *btn_stream_start;
    GtkWidget     *btn_stream_stop;

    // Displays
    GtkWidget     *term_view;
    GtkTextBuffer *term_buf;

    GtkWidget     *log_view;
    GtkTextBuffer *log_buf;

    GtkWidget     *img;

    GtkWidget     *status_lbl;
} App;

// ==============================
// Text append helpers
// ==============================
static void append_to_buffer(GtkTextBuffer *buf, const char *s) {
    if (!buf || !s) return;
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buf, &end);
    gtk_text_buffer_insert(buf, &end, s, -1);
    size_t n = strlen(s);
    if (n == 0 || s[n-1] != '\n') gtk_text_buffer_insert(buf, &end, "\n", -1);
}
static void gui_append_term(App *app, const char *msg){ append_to_buffer(app->term_buf, msg); }
static void gui_append_log (App *app, const char *msg){ append_to_buffer(app->log_buf,  msg); }

static void gui_handle_image(App *app, const char *remote_path){
    char line[512];
    g_snprintf(line, sizeof(line),
               "[IMG] Remote image on PAC-RF: %s\n[TODO] Fetch and render locally.\n",
               remote_path ? remote_path : "(null)");
    gui_append_log(app, line);
    (void)app;
}

// ==============================
// Main-thread marshaling
// ==============================
typedef enum { GUI_MSG_TERM, GUI_MSG_LOG, GUI_MSG_IMG, GUI_MSG_STATUS } GuiMsgKind;
typedef struct { App *app; GuiMsgKind kind; char *s; } GuiMsg;

static gboolean gui_dispatch_to_main(gpointer data){
    GuiMsg *m = (GuiMsg*)data;
    if(!m || !m->app){ if(m) g_free(m->s); g_free(m); return FALSE; }
    switch(m->kind){
        case GUI_MSG_TERM:   gui_append_term(m->app, m->s); break;
        case GUI_MSG_LOG:    gui_append_log (m->app, m->s); break;
        case GUI_MSG_IMG:    gui_handle_image(m->app, m->s); break;
        case GUI_MSG_STATUS: if(m->app->status_lbl) gtk_label_set_text(GTK_LABEL(m->app->status_lbl), m->s?m->s:""); break;
    }
    g_free(m->s); g_free(m); return FALSE;
}
static void post_gui(App *app, GuiMsgKind kind, const char *s){
    GuiMsg *m=g_new0(GuiMsg,1); m->app=app; m->kind=kind; m->s=g_strdup(s?s:""); g_idle_add(gui_dispatch_to_main,m);
}

// ==============================
// Output handler registry
// ==============================
typedef void (*OutputHandlerFn)(App*, const char*);
typedef struct OutputHandlerEntry { const char *prefix; OutputHandlerFn fn; struct OutputHandlerEntry *next; } OutputHandlerEntry;
static OutputHandlerEntry *g_handlers=NULL;

static void register_output_handler(const char *prefix, OutputHandlerFn fn){
    if(!prefix||!*prefix||!fn) return;
    OutputHandlerEntry *e=g_new0(OutputHandlerEntry,1); e->prefix=prefix; e->fn=fn; e->next=g_handlers; g_handlers=e;
}
static void oh_term(App *app,const char *p){ post_gui(app,GUI_MSG_TERM,p); }
static void oh_log (App *app,const char *p){ post_gui(app,GUI_MSG_LOG ,p); }
static void oh_img (App *app,const char *p){ char *t=g_strdup(p?p:""); g_strstrip(t); post_gui(app,GUI_MSG_IMG,t); g_free(t); }
static void oh_warn(App *app,const char *p){ char *t=g_strconcat("[WARN] ",p?p:"",NULL); post_gui(app,GUI_MSG_LOG,t); g_free(t); }
static void oh_err (App *app,const char *p){ char *t=g_strconcat("[ERROR] ",p?p:"",NULL); post_gui(app,GUI_MSG_LOG,t); g_free(t); }
static void oh_json(App *app,const char *p){ post_gui(app,GUI_MSG_LOG,p?p:""); }

static void init_default_handlers(void){
    register_output_handler("TERM: ", oh_term);
    register_output_handler("LOG: ",  oh_log);
    register_output_handler("IMG: ",  oh_img);
    // forward-looking
    register_output_handler("WARN: ", oh_warn);
    register_output_handler("ERR: ",  oh_err);
    register_output_handler("JSON: ", oh_json);
}

static void gui_route_line_registered(App *app, const char *line){
    for(OutputHandlerEntry *e=g_handlers; e; e=e->next){
        if(g_str_has_prefix(line, e->prefix)){
            e->fn(app, line + strlen(e->prefix));
            return;
        }
    }
    post_gui(app, GUI_MSG_LOG, line);
}
static void pacrf_on_line_cb(const char *line, void *user){
    gui_route_line_registered((App*)user, line);
}

// ==============================
// Workers
// ==============================
static gpointer gps_worker(gpointer u){ App *a=(App*)u; post_gui(a,GUI_MSG_STATUS,"Running GPS on PAC-RF…"); run_pacrf_cmd_cb("--gps",pacrf_on_line_cb,a); post_gui(a,GUI_MSG_STATUS,"GPS finished."); return NULL; }
static gpointer capture_worker(gpointer u){ App *a=(App*)u; post_gui(a,GUI_MSG_STATUS,"Running CAPTURE on PAC-RF…"); run_pacrf_cmd_cb("--capture",pacrf_on_line_cb,a); post_gui(a,GUI_MSG_STATUS,"Capture finished."); return NULL; }
static gpointer stream_start_worker(gpointer u){ App *a=(App*)u; post_gui(a,GUI_MSG_STATUS,"Starting STREAM on PAC-RF…"); run_pacrf_cmd_cb("--stream-start",pacrf_on_line_cb,a); post_gui(a,GUI_MSG_STATUS,"Stream start issued."); return NULL; }
static gpointer stream_stop_worker(gpointer u){ App *a=(App*)u; post_gui(a,GUI_MSG_STATUS,"Stopping STREAM on PAC-RF…"); run_pacrf_cmd_cb("--stream-stop",pacrf_on_line_cb,a); post_gui(a,GUI_MSG_STATUS,"Stream stop issued."); return NULL; }

// ==============================
// Button callbacks
// ==============================
static void on_btn_gps(GtkButton *b, gpointer u){ (void)b; App *a=(App*)u; gtk_text_buffer_set_text(a->term_buf,"", -1); gtk_text_buffer_set_text(a->log_buf,"", -1); g_thread_new("gps-worker",gps_worker,a); }
static void on_btn_capture(GtkButton *b, gpointer u){ (void)b; g_thread_new("capture-worker",capture_worker,(App*)u); }
static void on_btn_stream_start(GtkButton *b, gpointer u){ (void)b; g_thread_new("stream-start-worker",stream_start_worker,(App*)u); }
static void on_btn_stream_stop(GtkButton *b, gpointer u){ (void)b; g_thread_new("stream-stop-worker",stream_stop_worker,(App*)u); }

// ==============================
// UI helpers
// ==============================
static GtkWidget* make_textview_scrolled(GtkTextBuffer **out_buf){
    GtkWidget *scr = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scr), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scr, TRUE);
    gtk_widget_set_vexpand(scr, TRUE);

    GtkWidget *tv = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv), GTK_WRAP_WORD_CHAR);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scr), tv);
    if(out_buf) *out_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    return scr;
}

// ==============================
// Build UI (paned, spacious)
// ==============================
static void build_ui(App *app){
    // Window owned by GtkApplication
    app->window = gtk_application_window_new(app->gtk_app);
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1200, 800);
    gtk_window_set_title(GTK_WINDOW(app->window), "PAC-RF Control Center");

    // Header with buttons
    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), gtk_label_new("PAC-RF Control Center"));
    gtk_window_set_titlebar(GTK_WINDOW(app->window), header);

    app->btn_gps          = gtk_button_new_with_label("GPS");
    app->btn_capture      = gtk_button_new_with_label("Capture");
    app->btn_stream_start = gtk_button_new_with_label("Stream Start");
    app->btn_stream_stop  = gtk_button_new_with_label("Stream Stop");

    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), app->btn_gps);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), app->btn_capture);
    gtk_header_bar_pack_end  (GTK_HEADER_BAR(header), app->btn_stream_stop);
    gtk_header_bar_pack_end  (GTK_HEADER_BAR(header), app->btn_stream_start);

    // Root vertical box
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(root, 8);
    gtk_widget_set_margin_bottom(root, 8);
    gtk_widget_set_margin_start(root, 8);
    gtk_widget_set_margin_end(root, 8);
    gtk_window_set_child(GTK_WINDOW(app->window), root);

    // Status label
    app->status_lbl = gtk_label_new("Ready.");
    gtk_box_append(GTK_BOX(root), app->status_lbl);

    // Top-level horizontal paned: Left (text) | Right (image)
    GtkWidget *paned_h = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_hexpand(paned_h, TRUE);
    gtk_widget_set_vexpand(paned_h, TRUE);
    gtk_box_append(GTK_BOX(root), paned_h);

    // LEFT: vertical paned: Terminal (top) | Logs (bottom)
    GtkWidget *paned_v = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_hexpand(paned_v, TRUE);
    gtk_widget_set_vexpand(paned_v, TRUE);

    // Terminal
    GtkWidget *term_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *term_lbl = gtk_label_new("Terminal");
    GtkWidget *term_scr = make_textview_scrolled(&app->term_buf);
    gtk_box_append(GTK_BOX(term_box), term_lbl);
    gtk_box_append(GTK_BOX(term_box), term_scr);
    app->term_view = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(term_scr));

    // Logs
    GtkWidget *log_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *log_lbl = gtk_label_new("Logs");
    GtkWidget *log_scr = make_textview_scrolled(&app->log_buf);
    gtk_box_append(GTK_BOX(log_box), log_lbl);
    gtk_box_append(GTK_BOX(log_box), log_scr);
    app->log_view = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(log_scr));

    // Pack into vertical paned (give Terminal more height initially)
    gtk_paned_set_start_child(GTK_PANED(paned_v), term_box);
    gtk_paned_set_end_child  (GTK_PANED(paned_v), log_box);
    gtk_paned_set_position   (GTK_PANED(paned_v), 400); // initial divider (pixels)

    // RIGHT: Image panel
    GtkWidget *img_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *img_lbl = gtk_label_new("Image / Spectrum");
    app->img = gtk_image_new();
    gtk_widget_set_hexpand(app->img, TRUE);
    gtk_widget_set_vexpand(app->img, TRUE);
    gtk_box_append(GTK_BOX(img_box), img_lbl);
    gtk_box_append(GTK_BOX(img_box), app->img);

    // Pack into horizontal paned (give text side more width initially)
    gtk_paned_set_start_child(GTK_PANED(paned_h), paned_v);
    gtk_paned_set_end_child  (GTK_PANED(paned_h), img_box);
    gtk_paned_set_position   (GTK_PANED(paned_h), 800); // initial divider (pixels)

    // Wire callbacks
    g_signal_connect(app->btn_gps,          "clicked", G_CALLBACK(on_btn_gps),          app);
    g_signal_connect(app->btn_capture,      "clicked", G_CALLBACK(on_btn_capture),      app);
    g_signal_connect(app->btn_stream_start, "clicked", G_CALLBACK(on_btn_stream_start), app);
    g_signal_connect(app->btn_stream_stop,  "clicked", G_CALLBACK(on_btn_stream_stop),  app);

    // Initial text
    gtk_text_buffer_set_text(app->term_buf, "Click GPS to run the real PAC-RF handler remotely.\n", -1);
    gtk_text_buffer_set_text(app->log_buf,  "Logs will stream here.\n", -1);
}

// ==============================
// GTK application wiring
// ==============================
static void on_activate(GtkApplication *gtk_app, gpointer user){
    (void)user;
    init_default_handlers();

    App *app = g_new0(App,1);
    app->gtk_app = gtk_app;
    build_ui(app);
    gtk_window_present(GTK_WINDOW(app->window));
}

int main(int argc, char **argv){
    GtkApplication *gtk_app = gtk_application_new("com.arcane.pacrf", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(gtk_app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(gtk_app), argc, argv);
    g_object_unref(gtk_app);
    return status;
}// Build UI (paned, spacious)
// ==============================
static void build_ui(App *app){
    // Window owned by GtkApplication
    app->window = gtk_application_window_new(app->gtk_app);
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1200, 800);
    gtk_window_set_title(GTK_WINDOW(app->window), "PAC-RF Control Center");

    // Header with buttons
    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), gtk_label_new("PAC-RF Control Center"));
    gtk_window_set_titlebar(GTK_WINDOW(app->window), header);

    app->btn_gps          = gtk_button_new_with_label("GPS");
    app->btn_capture      = gtk_button_new_with_label("Capture");
    app->btn_stream_start = gtk_button_new_with_label("Stream Start");
    app->btn_stream_stop  = gtk_button_new_with_label("Stream Stop");

    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), app->btn_gps);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), app->btn_capture);
    gtk_header_bar_pack_end  (GTK_HEADER_BAR(header), app->btn_stream_stop);
    gtk_header_bar_pack_end  (GTK_HEADER_BAR(header), app->btn_stream_start);

    // Root vertical box
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(root, 8);
    gtk_widget_set_margin_bottom(root, 8);
    gtk_widget_set_margin_start(root, 8);
    gtk_widget_set_margin_end(root, 8);
    gtk_window_set_child(GTK_WINDOW(app->window), root);

    // Status label
    app->status_lbl = gtk_label_new("Ready.");
    gtk_box_append(GTK_BOX(root), app->status_lbl);

    // Top-level horizontal paned: Left (text) | Right (image)
    GtkWidget *paned_h = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_hexpand(paned_h, TRUE);
    gtk_widget_set_vexpand(paned_h, TRUE);
    gtk_box_append(GTK_BOX(root), paned_h);

    // LEFT: vertical paned: Terminal (top) | Logs (bottom)
    GtkWidget *paned_v = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_hexpand(paned_v, TRUE);
    gtk_widget_set_vexpand(paned_v, TRUE);

    // Terminal
    GtkWidget *term_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *term_lbl = gtk_label_new("Terminal");
    GtkWidget *term_scr = make_textview_scrolled(&app->term_buf);
    gtk_box_append(GTK_BOX(term_box), term_lbl);
    gtk_box_append(GTK_BOX(term_box), term_scr);
    app->term_view = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(term_scr));

    // Logs
    GtkWidget *log_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *log_lbl = gtk_label_new("Logs");
    GtkWidget *log_scr = make_textview_scrolled(&app->log_buf);
    gtk_box_append(GTK_BOX(log_box), log_lbl);
    gtk_box_append(GTK_BOX(log_box), log_scr);
    app->log_view = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(log_scr));

    // Pack into vertical paned (give Terminal more height initially)
    gtk_paned_set_start_child(GTK_PANED(paned_v), term_box);
    gtk_paned_set_end_child  (GTK_PANED(paned_v), log_box);
    gtk_paned_set_position   (GTK_PANED(paned_v), 400); // initial divider (pixels)

    // RIGHT: Image panel
    GtkWidget *img_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *img_lbl = gtk_label_new("Image / Spectrum");
    app->img = gtk_image_new();
    gtk_widget_set_hexpand(app->img, TRUE);
    gtk_widget_set_vexpand(app->img, TRUE);
    gtk_box_append(GTK_BOX(img_box), img_lbl);
    gtk_box_append(GTK_BOX(img_box), app->img);

    // Pack into horizontal paned (give text side more width initially)
    gtk_paned_set_start_child(GTK_PANED(paned_h), paned_v);
    gtk_paned_set_end_child  (GTK_PANED(paned_h), img_box);
    gtk_paned_set_position   (GTK_PANED(paned_h), 800); // initial divider (pixels)

    // Wire callbacks
    g_signal_connect(app->btn_gps,          "clicked", G_CALLBACK(on_btn_gps),          app);
    g_signal_connect(app->btn_capture,      "clicked", G_CALLBACK(on_btn_capture),      app);
    g_signal_connect(app->btn_stream_start, "clicked", G_CALLBACK(on_btn_stream_start), app);
    g_signal_connect(app->btn_stream_stop,  "clicked", G_CALLBACK(on_btn_stream_stop),  app);

    // Initial text
    gtk_text_buffer_set_text(app->term_buf, "Click GPS to run the real PAC-RF handler remotely.\n", -1);
    gtk_text_buffer_set_text(app->log_buf,  "Logs will stream here.\n", -1);
}

// ==============================
// GTK application wiring
// ==============================
static void on_activate(GtkApplication *gtk_app, gpointer user){
    (void)user;
    init_default_handlers();

    App *app = g_new0(App,1);
    app->gtk_app = gtk_app;
    build_ui(app);
    gtk_window_present(GTK_WINDOW(app->window));
}

int main(int argc, char **argv){
    GtkApplication *gtk_app = gtk_application_new("com.arcane.pacrf", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(gtk_app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(gtk_app), argc, argv);
    g_object_unref(gtk_app);
    return status;
}
