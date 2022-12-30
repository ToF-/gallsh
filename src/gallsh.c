#include <gtk/gtk.h>
#include <stdio.h>
#include <wordexp.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#ifdef __linux__
#ifndef G_APPLICATION_DEFAULT_FLAGS
#define G_APPLICATION_DEFAULT_FLAGS (G_APPLICATION_FLAGS_NONE)
#endif
#endif

char Image_Directory[4096] = "images/";

typedef struct user_data {
    char **filenames;
    int *times_viewed;
    char *selected_filename;
    int count;
    int selected;
    int views;
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
    int search = true;
    if(data->count > 1)
        do{
            search = true;
            data->selected = rand() % data->count;
            g_print("selecting %d\n", data->selected);
            if(data->selected != old 
                    && (data->times_viewed[data->selected] == 0 
                    || data->views >= data->count)) 
                search = false;
                if(search)
                g_print("repeating, reselect\n");
        }while(search);
    else
        data->selected = 0;
    data->selected_filename = data->filenames[data->selected];
    data->times_viewed[data->selected]++;
    data->views++;
    g_print("%d\t%d\t%d\t%s\n", data->views, data->selected, data->times_viewed[data->selected], data->selected_filename);
}

void select_random_image(USER_DATA *data) {
    select_random(data);
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

void get_image_directory() {
    char *value = getenv("GALLSHDIR");
    wordexp_t exp_result;
    if(value != NULL) {
        strcpy(Image_Directory, value);
        int l = strlen(Image_Directory);
        if(Image_Directory[l-1] != '/')
            strcat(Image_Directory, "/");
        wordexp(Image_Directory, &exp_result, 0);
        strcpy(Image_Directory, exp_result.we_wordv[0]);
    } else 
        printf("variable GALLSHDIR not found\n");
    printf("%s is image directory\n", Image_Directory);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;
    get_image_directory();
    srand(time(NULL));
    USER_DATA *data = (USER_DATA *)malloc(sizeof(USER_DATA));
    data->count = count_directory_entries(Image_Directory);
    data->views = 0;
    if(data->count == 0) {
        fprintf(stderr, "no file found in the directory %s\n", Image_Directory);
        return 1;
    }
    g_print("%d images in the directorny\n", data->count);
    data->filenames = (char **)malloc(sizeof(char *) * data->count);
    data->times_viewed = (int *)malloc(sizeof(int) * data->count);
    for(int i=0; i < data->count; data->times_viewed[i++] = 0);
    read_filenames(data->filenames, Image_Directory);
    app = gtk_application_new(NULL, G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(app, "activate", G_CALLBACK(app_activate), data);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    destroy_filenames(data->filenames, data->count);
    free(data);
    return status;
}
