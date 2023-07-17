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

#define MAX_IMAGES 10000
#define PNG ".png"
#define JPG ".jpg"
#define JPEG ".jpeg"

#define MAX_FILE_PATH 1024
#define DIRECTORY 'd'
#define RECURSIVE 'r'
#define HELP      'h'
#define MAXIMIZE  'm'
#define NO_RANDOM 'o'
#define PATTERN   'p'
#define TIMER     't'
#define NEXT      'n'
#define PREVIOUS  'p'
#define QUIT      'q'
#define SPACE     ' '
#define RANDOM    'r'

char Image_Directory[4096] = "images/";

typedef struct parameters {
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
    bool recursive;
    int  timer;
    guint        timeout_id;
    GtkImage     *image;
    GApplication *application;

} PARAMETERS;

int count_directory_entries(char *dirname, char *pattern, bool recursive);
int read_filenames(char **entries, char *dirname, char *pattern, bool recursive, int count);
char *random_filename(char **entries, int count, int *selected);
void destroy_filenames(char **entries, int count);
void select_random_image(PARAMETERS *p);
void load_image(PARAMETERS *p);
bool valid_image_file(char *name);


bool valid_image_file(char *name) {
    char *ext = strrchr(name, '.');
    if(!ext)
        return false;
    return (!strcmp(PNG, ext) || !strcmp(JPG, ext) || !strcmp(JPEG, ext));
}

int count_directory_entries(char *dirname, char *pattern, bool recursive) {
    printf(".");
    DIR *directory;
    struct dirent *entry;
    directory = opendir(dirname);
    int count = 0;
    if(directory) {
        while((entry = readdir(directory)) != NULL)
            if(entry->d_type != DT_DIR) {
                if(!pattern || (strstr(entry->d_name, pattern)))
                    if(valid_image_file(entry->d_name))
                        count++;
            }
            else {
                if(recursive && strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
                    char filePath[MAX_FILE_PATH];
                    snprintf(filePath, sizeof(filePath), "%s/%s", dirname, entry->d_name);
                    count += count_directory_entries(filePath, pattern, recursive);
                }
            }
        closedir(directory);
    }
    return count;
}

gboolean on_timeout_event(gpointer p) {
    select_random_image(p);
    load_image(p);
    return TRUE;
}


int compare(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

int read_filenames(char **entries, char *dirname, char *pattern, bool recursive, int count) {
    DIR *directory;
    struct dirent *entry;
    directory = opendir(dirname);
    if(directory) {
        while((entry = readdir(directory)) != NULL)
            if(entry->d_type != DT_DIR) {
                if(!pattern || strstr(entry->d_name, pattern)) {
                    char *entry_name = (char *)malloc(strlen(dirname) + strlen(entry->d_name) + 1);
                    if(valid_image_file(entry->d_name)) {
                        strcpy(entry_name, dirname);
                        strcat(entry_name, entry->d_name);
                        entries[count] = entry_name;
                        count++;
                    }
                }
            }
            else {
                if(recursive && strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
                    char *new_dirname = (char *)malloc(strlen(dirname) + strlen(entry->d_name) + 2);
                    strcpy(new_dirname, dirname);
                    strcat(new_dirname, entry->d_name);
                    strcat(new_dirname, "/");
                    count = read_filenames(entries, new_dirname, pattern, recursive, count);
                    free(new_dirname);
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

void select_next_image(PARAMETERS *p) {
    if(!p->count)
        return;
    p->selected++;
    if(p->selected == p->count)
        p->selected = 0;
}

void select_prev_image(PARAMETERS *p) {
    if(!p->count)
        return;
    p->selected--;
    if(p->selected < 0)
        p->selected = p->count-1;;
}
void select_random_image(PARAMETERS *p) {
    if(p->count < 2 || p->random == false) {
        p->selected = 0;
        return;
    }
    int old = p->selected;
    int keep_searching = true;
    do{
        p->selected = rand() % p->count;
        if(p->selected != old
                && (p->times_viewed[p->selected] == 0
                    || p->views >= p->count))
            keep_searching = false;
    }while(keep_searching);
}

void load_image(PARAMETERS *p) {
    p->selected_filename = p->filenames[p->selected];
    p->times_viewed[p->selected]++;
    p->views++;
    assert(p->selected_filename);
    gtk_image_set_from_file(p->image, p->selected_filename);

    gtk_widget_queue_draw (GTK_WIDGET(gtk_widget_get_parent(GTK_WIDGET(p->image))));
    gtk_window_set_title(GTK_WINDOW(gtk_application_get_active_window((GtkApplication *)p->application)), p->selected_filename);
}

gboolean key_pressed ( GtkEventControllerKey* self, guint keyval, guint keycode, GdkModifierType* state, gpointer gp) {
    PARAMETERS *p = (PARAMETERS *)gp;
    if(keyval == QUIT)
        g_application_quit(p->application);
    else if(p->random && keyval == SPACE || keyval == RANDOM)
        select_random_image(p);
    else if(keyval == 'n' || keyval == ' ')
        select_next_image(p);
    else if(keyval == 'p')
        select_prev_image(p);
    load_image(p);
    return true;
}
static void app_activate(GApplication *app, gpointer gp) {
    GtkWidget *window;
    GtkImage *image;
    GdkDisplay *display;
    GtkCssProvider *css_provider;
    GtkEventController *event_controller;

    window  = gtk_application_window_new(GTK_APPLICATION(app));
    image   = GTK_IMAGE(gtk_image_new());
    PARAMETERS *p = (PARAMETERS *)gp;
    p->application = app;
    p->image = image;
    display = gtk_widget_get_display(GTK_WIDGET(window));
    css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider, "window { background-color:black; } image { margin:10em; }", -1);
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    event_controller = gtk_event_controller_key_new();
    g_signal_connect(event_controller, "key-pressed", G_CALLBACK(key_pressed), p);
    gtk_widget_add_controller(window, event_controller);
    gtk_window_set_child(GTK_WINDOW(window), GTK_WIDGET(image));
    gtk_window_set_title(GTK_WINDOW(window), "gallsh");
    gtk_window_set_decorated(GTK_WINDOW(window), true);
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 1000);
    gtk_window_set_resizable(GTK_WINDOW(window), true);
    load_image(p);
    if(p->maximized)
    {
        int width;
        int height;
        gtk_window_get_default_size((GtkWindow *)window, &width, &height);
        printf("size: %d x %d \n", width, height);
        gtk_window_fullscreen((GtkWindow *)window);
    }
    if(p->timer) {
        p->timeout_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 1000*p->timer, (GSourceFunc)on_timeout_event, p, NULL);
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

PARAMETERS *new_parameters() {
    PARAMETERS *p = (PARAMETERS *)malloc(sizeof(PARAMETERS));
    p->pattern = NULL;
    p->directory = NULL;
    p->views = 0;
    p->random = true;
    p->maximized = false;
    p->recursive = false;
    p->timer = 0;
    return p;
}

void free_parameters(PARAMETERS *p) {
    if(p->directory)
        free(p->directory);
    if(p->pattern)
        free(p->pattern);
    for(int i=0; i < p->count; i++)
        free(p->filenames[i]);
    free(p);
}

bool valid_command(char c) {
  return (   c == DIRECTORY
          || c == HELP
          || c == MAXIMIZE
          || c == RECURSIVE
          || c == NO_RANDOM
          || c == PATTERN
          || c == TIMER );

}
void get_image_filenames(PARAMETERS *p) {
    p->filenames = (char **)malloc(sizeof(char *) * p->count);
    p->times_viewed = (int *)malloc(sizeof(int) * p->count);
    for(int i=0; i < p->count; p->times_viewed[i++] = 0);
    int count = read_filenames(p->filenames, Image_Directory, p->pattern, p->recursive, 0);
    assert(count == p->count);
}

int get_parameters(PARAMETERS *p, int argc, char **argv) {
    if(argc == 2 && argv[1][0] != '-') {
        p->pattern = (char *)malloc(strlen(argv[1])+1);
        strcpy(p->pattern, argv[1]);
        return 0;
    }
    char option;
    for(int i=1; i<argc; i++) {
        if(strlen(argv[i]) != 2 || argv[i][0] != '-' || !valid_command(argv[i][1])) {
            fprintf(stderr, "usage: gallsh -%c <directory> -%c <pattern> -%c (recursive) -%c (no random) -%c (maximized) -%c <seconds> -%c (help)\n",
                    DIRECTORY, PATTERN, RECURSIVE, NO_RANDOM, MAXIMIZE, TIMER, HELP);
            return 1;
        }
        option = argv[i][1];
        switch(option) {
            case NO_RANDOM :
                p->random = false;
                break;
            case RECURSIVE :
                p->recursive = true;
                break;
            case MAXIMIZE :
                p->maximized = true;
                break;
            case HELP :
                fprintf(stderr, "usage: gallsh -d <directory> -p <pattern> -r (no random) -m (maximized) -t <seconds> -h (help)\n");
                return 1;
            case DIRECTORY :
                i++;
                if (i >= argc) {
                    fprintf(stderr, "missing directory argument\n");
                    return 1;
                }
                p->directory = (char *)malloc(strlen(argv[i])+1);
                strcpy(p->directory, argv[i]);
                break;
            case PATTERN :
                i++;
                if (i >= argc) {
                    fprintf(stderr, "missing pattern argument\n");
                    return 1;
                }
                p->pattern = (char *)malloc(strlen(argv[i])+1);
                strcpy(p->pattern, argv[i]);
                break;
            case TIMER :
                i++;
                if (i >= argc) {
                    fprintf(stderr, "missing seconds argument\n");
                    free_parameters(p);
                    return 1;
                }
                p->timer = atoi(argv[i]);
                break;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;
    srand(time(NULL));
    PARAMETERS *p = new_parameters();
    if(get_parameters(p, argc, argv)) {
        free_parameters(p);
        return 1;
    }
    gtk_init();
    get_image_directory(p->directory);
    p->count = count_directory_entries(Image_Directory, p->pattern, p->recursive);
    if(p->count == 0) {
        fprintf(stderr, "\nno image found\n");
        return 1;
    }
    if(p->count >= MAX_IMAGES) {
        fprintf(stderr, "\nmax image limit (%d/%d) exceeded\n", p->count, MAX_IMAGES);
        return 1;
    }
    g_print("\n%d images found\n", p->count);

    get_image_filenames(p);
    select_random_image(p);
    app = gtk_application_new(NULL, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), p);
    status = g_application_run(G_APPLICATION(app), 0, NULL);

    if(p->timer) {
        g_source_remove(p->timeout_id);
    }
    g_object_unref(app);
    destroy_filenames(p->filenames, p->count);
    free(p);
    return status;
}
