#include <gtk/gtk.h>
#include <stdio.h>
#include <wordexp.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>

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
    bool random;
    bool maximized;
    GtkImage *image;
    GApplication *application;

} USER_DATA;

int count_directory_entries(char *dirname, char *pattern);
int read_filenames(char **entries, char *dirname, char *pattern);
char *random_filename(char **entries, int count, int *selected);
void destroy_filenames(char **entries, int count);

int count_directory_entries(char *dirname, char *pattern) {
    printf("counting entries ");
    if(pattern != NULL)
        printf("with pattern %s ",pattern);
    DIR *directory;
    struct dirent *entry;
    directory = opendir(dirname);
    int count = 0;
    if(directory) {
        while((entry = readdir(directory)) != NULL)
            if(entry->d_name[0] != '.')
                if(!pattern || (strstr(entry->d_name, pattern)))
                    count++;
        closedir(directory);
    }
    printf("%d\n", count);
    return count;
}

int compare(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

int read_filenames(char **entries, char *dirname, char *pattern) {
    DIR *directory;
    struct dirent *entry;
    directory = opendir(dirname);
    int count = 0;
    if(directory) {
        while((entry = readdir(directory)) != NULL)
            if(entry->d_name[0] != '.') {
                if(!pattern || strstr(entry->d_name, pattern)) {
                    char *entry_name = (char *)malloc(strlen(dirname) + strlen(entry->d_name) + 1);
                    strcpy(entry_name, dirname);
                    strcat(entry_name, entry->d_name);
                    entries[count] = entry_name;
                    printf("%d %s\n", count, entries[count]);
                    count++;
                }
            }
        closedir(directory);
    }
    qsort(entries, count, sizeof(const char *), compare);
    return count;
}

void destroy_filenames(char **entries, int count) {
    for(int i=0; i < count; i++) {
        free(entries[i]);
    }
}

void select_next_image(USER_DATA *data) {
    if(!data->count)
        return;
    data->selected++;
    if(data->selected == data->count)
        data->selected = 0;
}

void select_prev_image(USER_DATA *data) {
    if(!data->count)
        return;
    data->selected--;
    if(data->selected < 0)
        data->selected = data->count-1;;
}
void select_random_image(USER_DATA *data) {
    if(!data->count)
        return;
    if(data->count == 1) {
        data->selected = 0;
        return;
    }
    int old = data->selected;
    int search = true;
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
}

void load_image(USER_DATA *data) {
    data->selected_filename = data->filenames[data->selected];
    data->times_viewed[data->selected]++;
    data->views++;
    g_print("%d\t%d\t%d\t%s\n", data->views, data->selected, data->times_viewed[data->selected], data->selected_filename);
    assert(data->selected_filename);
    gtk_image_set_from_file(data->image, data->selected_filename);
    gtk_widget_queue_draw (GTK_WIDGET(gtk_widget_get_parent(GTK_WIDGET(data->image))));
}

gboolean key_pressed ( GtkEventControllerKey* self, guint keyval, guint keycode, GdkModifierType* state, gpointer user_data) {
    USER_DATA *data = (USER_DATA *)user_data;
    if(keyval == 'q')
        g_application_quit(data->application);
    else if(data->random && keyval == ' ' || keyval == 'r')
        select_random_image(data);
    else if(keyval == 'n' || keyval == ' ')
        select_next_image(data);
    else if(keyval == 'p')
        select_prev_image(data);
    load_image(data);
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
    gtk_window_set_decorated(GTK_WINDOW(window), true);
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 1000);
    gtk_window_set_resizable(GTK_WINDOW(window), true);
    load_image(data);
    if(data->maximized)
        gtk_window_maximize(GTK_WINDOW(window));
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
    gtk_init();
    char *pattern = NULL;
    USER_DATA *data = (USER_DATA *)malloc(sizeof(USER_DATA));
    data->views = 0;
    data->random = true;
    data->maximized = false;
    for(int i=1; i<argc; i++) {
        if(!strcmp(argv[i], "no-random"))
            data->random = false;
        else if(!strcmp(argv[i], "maximized"))
            data->maximized = true;
        else if(!strcmp(argv[i], "pattern")) {
            if(argc < (i+1)) {
                fprintf(stderr, "usage: gallsh pattern <pattern>\n");
                exit(1);
            }
            pattern = strdup(argv[i+1]);
            break;
        }
        else {
            fprintf(stderr, "%s ?\n", argv[i]);
            exit(1);
        }
    }
    data->count = count_directory_entries(Image_Directory, pattern);
    if(data->count == 0) {
        if(pattern)
            fprintf(stderr, "no file found in the directory %s for pattern %s\n", Image_Directory, pattern);
        else
            fprintf(stderr, "no file found in the directory %s\n", Image_Directory);
        return 1;
    }
    if(pattern)
        g_print("%d images in the directory for pattern %s\n", data->count, pattern);
    else
        g_print("%d images in the directory\n", data->count);

    data->filenames = (char **)malloc(sizeof(char *) * data->count);
    data->times_viewed = (int *)malloc(sizeof(int) * data->count);
    for(int i=0; i < data->count; data->times_viewed[i++] = 0);
    int count = read_filenames(data->filenames, Image_Directory, pattern);
    assert(count == data->count);
    select_random_image(data);
    app = gtk_application_new(NULL, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), data);
    status = g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);
    destroy_filenames(data->filenames, data->count);
    free(data);
    if(pattern)
        free(pattern);
    return status;
}
