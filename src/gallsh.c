#include <gtk/gtk.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>

#ifdef __linux__
#ifndef G_APPLICATION_DEFAULT_FLAGS
#define G_APPLICATION_DEFAULT_FLAGS (G_APPLICATION_FLAGS_NONE)
#endif
#endif

typedef struct user_data {
    char **filenames;
    char *selected_filename;
    int count;
    int selected;
    GtkImage *image;
    GApplication *application;

} USER_DATA;

int count_directory_entries(char *dirname);
int read_filenames(char **entries, char *dirname);
char *random_filename(char **entries, int count, int *selected);
void destroy_filenames(char **entries, int count);

int count_directory_entries(char *dirname) {
    DIR *directory;
    struct dirent *entry;
    directory = opendir(dirname);
    int count = 0;
    if(directory) {
        while((entry = readdir(directory)) != NULL)
            if(entry->d_name[0] != '.')
                count++;
        closedir(directory);
    }
    return count;
}

int read_filenames(char **entries, char *dirname) {
    DIR *directory;
    struct dirent *entry;
    directory = opendir(dirname);
    int count = 0;
    if(directory) {
        while((entry = readdir(directory)) != NULL)
            if(entry->d_name[0] != '.') {
                char *entry_name = (char *)malloc(strlen(dirname) + strlen(entry->d_name) + 1);
                strcpy(entry_name, dirname);
                strcat(entry_name, entry->d_name);
                entries[count++] = entry_name;
            }
        closedir(directory);
    }
    return count;
}

void destroy_filenames(char **entries, int count) {
    for(int i=0; i < count; i++) {
        free(entries[i]);
    }
}

void select_random(USER_DATA *data) {
    int old = data->selected;
    do{
        data->selected = rand() % data->count;
    }while(data->selected == old);
    data->selected_filename = data->filenames[data->selected];
}

void select_random_image(USER_DATA *data) {
    select_random(data);
    g_print("%d:%s\n", data->selected, data->selected_filename);
    gtk_image_set_from_file(data->image, data->selected_filename);
    gtk_widget_queue_draw (GTK_WIDGET(gtk_widget_get_parent(GTK_WIDGET(data->image))));
}

gboolean key_pressed ( GtkEventControllerKey* self, guint keyval, guint keycode, GdkModifierType* state, gpointer user_data) {
    USER_DATA *data = (USER_DATA *)user_data;
    if(keyval == 'q')
        g_application_quit(data->application);
    else if(keyval == ' ')
        select_random_image(data);
    return true;
}
static void app_activate(GApplication *app, gpointer *user_data) {
    GtkWidget *window;
    GtkImage *image;
    GdkDisplay *display;
    GtkCssProvider *css_provider;
    GtkEventController *event_controller;

    window  = gtk_application_window_new(GTK_APPLICATION(app));
    image   = GTK_IMAGE(gtk_image_new());
    USER_DATA *data = (USER_DATA *)user_data;
    data->application = app;
    data->image = image;
    display = gtk_widget_get_display(GTK_WIDGET(window));
    css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider, "window { background-color:black; } image { margin:10em; }", -1);
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    event_controller = gtk_event_controller_key_new();
    g_signal_connect(event_controller, "key-pressed", G_CALLBACK(key_pressed), user_data);
    gtk_widget_add_controller(window, event_controller);
    gtk_window_set_child(GTK_WINDOW(window), GTK_WIDGET(image));
    gtk_window_set_title(GTK_WINDOW(window), "gallsh");
    gtk_window_set_decorated(GTK_WINDOW(window), false);
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 1000);

    select_random_image(data);
    gtk_widget_show(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;
    srand(time(NULL));
    USER_DATA *data = (USER_DATA *)malloc(sizeof(USER_DATA));
    data->count = count_directory_entries("images/");
    data->filenames = (char **)malloc(sizeof(char *) * data->count);
    read_filenames(data->filenames, "images/");
    app = gtk_application_new(NULL, G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(app, "activate", G_CALLBACK(app_activate), data);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    destroy_filenames(data->filenames, data->count);
    free(data);
    return status;
}
