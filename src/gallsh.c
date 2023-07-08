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

#define DIRECTORY 'd'
#define HELP      'h'
#define MAXIMIZE  'm'
#define NO_RANDOM 'r'
#define PATTERN   'p'
#define TIMER     't'
#define NEXT      'n'
#define PREVIOUS  'p'
#define QUIT      'q'
#define SPACE     ' '
#define RANDOM    'r'

char Image_Directory[4096] = "images/";

typedef struct user_data {
    char *directory;
    char *pattern;
    char **filenames;
    int  *times_viewed;
    char *selected_filename;
    int  count;
    int  selected;
    int  views;
    bool random;
    bool maximized;
    int  timer;
    guint        timeout_id;
    GtkImage     *image;
    GApplication *application;

} USER_DATA;

void init_user_data();
int count_directory_entries(char *dirname, char *pattern);
int read_filenames(char **entries, char *dirname, char *pattern);
char *random_filename(char **entries, int count, int *selected);
void destroy_filenames(char **entries, int count);
void select_random_image(USER_DATA *ud);
void load_image(USER_DATA *ud);

int count_directory_entries(char *dirname, char *pattern) {
    printf("counting entries ");
    DIR *directory;
    if(pattern != NULL)
        printf("with pattern %s ",pattern);
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

gboolean on_timeout_event(gpointer ud) {
    select_random_image(ud);
    load_image(ud);
    return TRUE;
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

void select_next_image(USER_DATA *ud) {
    if(!ud->count)
        return;
    ud->selected++;
    if(ud->selected == ud->count)
        ud->selected = 0;
}

void select_prev_image(USER_DATA *ud) {
    if(!ud->count)
        return;
    ud->selected--;
    if(ud->selected < 0)
        ud->selected = ud->count-1;;
}
void select_random_image(USER_DATA *ud) {
    if(!ud->count)
        return;
    if(ud->count == 1) {
        ud->selected = 0;
        return;
    }
    int old = ud->selected;
    int search = true;
    do{
        search = true;
        ud->selected = rand() % ud->count;
        g_print("selecting %d\n", ud->selected);
        if(ud->selected != old 
                && (ud->times_viewed[ud->selected] == 0 
                    || ud->views >= ud->count)) 
            search = false;
        if(search)
            g_print("repeating, reselect\n");
    }while(search);
}

void load_image(USER_DATA *ud) {
    ud->selected_filename = ud->filenames[ud->selected];
    ud->times_viewed[ud->selected]++;
    ud->views++;
    g_print("%d\t%d\t%d\t%s\n", ud->views, ud->selected, ud->times_viewed[ud->selected], ud->selected_filename);
    assert(ud->selected_filename);
    gtk_image_set_from_file(ud->image, ud->selected_filename);

    gtk_widget_queue_draw (GTK_WIDGET(gtk_widget_get_parent(GTK_WIDGET(ud->image))));
    gtk_window_set_title(GTK_WINDOW(gtk_application_get_active_window((GtkApplication *)ud->application)), ud->selected_filename);
}

gboolean key_pressed ( GtkEventControllerKey* self, guint keyval, guint keycode, GdkModifierType* state, gpointer p) {
    USER_DATA *ud = (USER_DATA *)p;
    if(keyval == QUIT)
        g_application_quit(ud->application);
    else if(ud->random && keyval == SPACE || keyval == RANDOM)
        select_random_image(ud);
    else if(keyval == 'n' || keyval == ' ')
        select_next_image(ud);
    else if(keyval == 'p')
        select_prev_image(ud);
    load_image(ud);
    return true;
}
static void app_activate(GApplication *app, gpointer p) {
    GtkWidget *window;
    GtkImage *image;
    GdkDisplay *display;
    GtkCssProvider *css_provider;
    GtkEventController *event_controller;

    window  = gtk_application_window_new(GTK_APPLICATION(app));
    image   = GTK_IMAGE(gtk_image_new());
    USER_DATA *ud = (USER_DATA *)p;
    ud->application = app;
    ud->image = image;
    display = gtk_widget_get_display(GTK_WIDGET(window));
    css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider, "window { background-color:black; } image { margin:10em; }", -1);
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    event_controller = gtk_event_controller_key_new();
    g_signal_connect(event_controller, "key-pressed", G_CALLBACK(key_pressed), ud);
    gtk_widget_add_controller(window, event_controller);
    gtk_window_set_child(GTK_WINDOW(window), GTK_WIDGET(image));
    gtk_window_set_title(GTK_WINDOW(window), "gallsh");
    gtk_window_set_decorated(GTK_WINDOW(window), true);
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 1000);
    gtk_window_set_resizable(GTK_WINDOW(window), true);
    load_image(ud);
    if(ud->maximized)
    {
        int width;
        int height;
        gtk_window_get_default_size((GtkWindow *)window, &width, &height);
        printf("size: %d x %d \n", width, height);
        gtk_window_fullscreen((GtkWindow *)window);
    }
    if(ud->timer) {
        ud->timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 1000*ud->timer, (GSourceFunc)on_timeout_event, ud, NULL);
    }
    gtk_widget_set_visible(window, true);
}

void get_image_directory(char *filepath) {
    char *value = filepath ? filepath : getenv("GALLSHDIR");
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

USER_DATA *new_user_data() {
    USER_DATA *ud = (USER_DATA *)malloc(sizeof(USER_DATA));
    ud->pattern = NULL;
    ud->directory = NULL;
    ud->views = 0;
    ud->random = true;
    ud->maximized = false;
    ud->timer = 0;
    return ud;
}

void free_user_data(USER_DATA *ud) {
    if(ud->directory)
        free(ud->directory);
    if(ud->pattern)
        free(ud->pattern);
    for(int i=0; i < ud->count; i++)
        free(ud->filenames[i]);
    free(ud);
}

bool valid_command(char c) {
  return (   c == DIRECTORY
          || c == HELP
          || c == MAXIMIZE
          || c == NO_RANDOM
          || c == PATTERN
          || c == TIMER );

}
void init_user_data(USER_DATA *ud) {
    ud->filenames = (char **)malloc(sizeof(char *) * ud->count);
    ud->times_viewed = (int *)malloc(sizeof(int) * ud->count);
    for(int i=0; i < ud->count; ud->times_viewed[i++] = 0);
    int count = read_filenames(ud->filenames, Image_Directory, ud->pattern);
    assert(count == ud->count);
    select_random_image(ud);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;
    srand(time(NULL));
    USER_DATA *ud = new_user_data();
    gtk_init();
    char option;
    for(int i=1; i<argc; i++) {
        if(argc == 2 && argv[1][0] != '-') {
            option = PATTERN;
            ud->pattern = (char *)malloc(strlen(argv[i])+1);
            strcpy(ud->pattern, argv[i]);
        }
        else { 
            if(strlen(argv[i]) != 2 || argv[i][0] != '-' || !valid_command(argv[i][1])) {
                fprintf(stderr, "usage: gallsh -d <directory> -p <pattern> -r (no random) -m (maximized) -t <seconds> -h (help)\n");
                free_user_data(ud);
                exit(1);
            }
            option = argv[i][1];
            switch(option) {
                case DIRECTORY :
                    i++;
                    if (i >= argc) {
                        fprintf(stderr, "missing directory argument\n");
                        free_user_data(ud);
                        exit(1);
                    }
                    ud->directory = (char *)malloc(strlen(argv[i])+1);
                    strcpy(ud->directory, argv[i]);
                    break;
                case NO_RANDOM :
                    ud->random = false;
                    break;
                case PATTERN :
                    i++;
                    if (i >= argc) {
                        fprintf(stderr, "missing pattern argument\n");
                        free_user_data(ud);
                        exit(1);
                    }
                    ud->pattern = (char *)malloc(strlen(argv[i])+1);
                    strcpy(ud->pattern, argv[i]);
                    break;
                case TIMER :
                    i++;
                    if (i >= argc) {
                        fprintf(stderr, "missing seconds argument\n");
                        free_user_data(ud);
                        exit(1);
                    }
                    ud->timer = atoi(argv[i]);
                    break;
                case MAXIMIZE :
                    ud->maximized = true;
                    break;
                case HELP :
                    fprintf(stderr, "usage: gallsh -d <directory> -p <pattern> -r (no random) -m (maximized) -t <seconds> -h (help)\n");
                    free_user_data(ud);
                    exit(0);
            }
        }
    }
    get_image_directory(ud->directory);
    ud->count = count_directory_entries(Image_Directory, ud->pattern);
    if(ud->count == 0) {
        if(ud->pattern)
            fprintf(stderr, "no file found in the directory %s for pattern %s\n", Image_Directory, ud->pattern);
        else
            fprintf(stderr, "no file found in the directory %s\n", Image_Directory);
        return 1;
    }
    if(ud->pattern)
        g_print("%d images in the directory for pattern %s\n", ud->count, ud->pattern);
    else
        g_print("%d images in the directory\n", ud->count);

    init_user_data(ud);
    app = gtk_application_new(NULL, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), ud);
    status = g_application_run(G_APPLICATION(app), 0, NULL);

    if(ud->timer) {
        g_source_remove(ud->timeout_id);
    }
    g_object_unref(app);
    destroy_filenames(ud->filenames, ud->count);
    free(ud);
    return status;
}
