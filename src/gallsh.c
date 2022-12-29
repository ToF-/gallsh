#include <gtk/gtk.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>

typedef struct user_data {
    char **filenames;
    int count;
} USER_DATA;

int count_directory_entries(char *dirname);
int read_filenames(char **entries, char *dirname);
char *random_filename(char **entries, int count);
void destroy_filenames(char **entries, int count);

char *filename;

static void app_activate(GApplication *app, gpointer *user_data) {
    GtkWidget *window;
    GtkWidget *image;
    GdkDisplay *display;
    GtkCssProvider *css_provider;
    USER_DATA *data = (USER_DATA *)user_data;

    window  = gtk_application_window_new(GTK_APPLICATION(app));
    image   = gtk_image_new_from_file(random_filename(data->filenames, data->count));
    display = gtk_widget_get_display(GTK_WIDGET(window));
    css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider, "window { background-color:black; } image { margin:10em; }", -1);
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    gtk_window_set_child(GTK_WINDOW(window), image);
    gtk_window_set_title(GTK_WINDOW(window), "gallsh");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 1000);

    gtk_widget_show(window);
}

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

char *random_filename(char **entries, int count) {
    int selected = rand() % count;
    return entries[selected];
}
void destroy_filenames(char **entries, int count) {
    for(int i=0; i < count; i++) {
        free(entries[i]);
    }
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
