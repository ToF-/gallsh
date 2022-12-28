#include <gtk/gtk.h>
#include <stdio.h>

static void app_activate(GApplication *app, gpointer *user_data) {
    GtkWidget *window;
    GtkWidget *image;
    GdkDisplay *display;
    GtkCssProvider *css_provider;

    window  = gtk_application_window_new(GTK_APPLICATION(app));
    image   = gtk_image_new_from_file("images/alphabet-vr-vasarely.jpg");
    display = gtk_widget_get_display(GTK_WIDGET(window));
    css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider, "window { background-color:black; }", -1);
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    gtk_window_set_child(GTK_WINDOW(window), image);
    gtk_window_set_title(GTK_WINDOW(window), "gallsh");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 1000);

    gtk_widget_show(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new(NULL, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
