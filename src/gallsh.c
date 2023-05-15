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
    char *directory;
    char *pattern;
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
    gtk_window_set_title(GTK_WINDOW(gtk_application_get_active_window(ud->application)), ud->selected_filename);
}

gboolean key_pressed ( GtkEventControllerKey* self, guint keyval, guint keycode, GdkModifierType* state, gpointer p) {
    USER_DATA *ud = (USER_DATA *)p;
    if(keyval == 'q')
        g_application_quit(ud->application);
    else if(ud->random && keyval == ' ' || keyval == 'r')
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
        gtk_window_maximize(GTK_WINDOW(window));
    gtk_widget_show(window);
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

(USER_DATA *)new_user_data() {
    USER_DATA *ud = (USER_DATA *)malloc(sizeof(USER_DATA));
    ud->pattern = NULL;
    ud->directory = NULL;
    ud->views = 0;
    ud->random = true;
    ud->maximized = false;
    return ud;
}

void free_user_data(USER_DATA *ud) {
    if(ud->directory)
        free(ud->directory);
    if(ud->pattern)
        free(ud->pattern);
    for(int i=0; i < ud->count; i++)
        free(ud->filenames[i];
    free(ud);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;
    srand(time(NULL));
    (USER_DATA *)ud = new_user_data();
    gtk_init();
    for(int i=1; i<argc; i++) {
        if(strlen(argv[i] != 2 || argv[i][0] != '-' || (argv[i] != 'd' && argv[i] != 'p' && argv[i] != 'r' && argv[i] != 'm' && argv[i] != 'h') {
            fprintf(stderr, "usage: gallsh -d <directory> -p <pattern> -r (no random) -m (maximized) -h (help)\n")
                free_user_data(ud);
                exit(1);
        }
        char option = argv[i][1];
        switch option {
        case 'd' :
            i++;
            if (i >= 

        if(!strcmp(argv[i], "-r"))
            ud->random = false;
        if(!strcmp(argv[i], "-a")) {
            if(i < (argc-1)) {
                image_directory = argv[i+1];
                i++;
            }
            else {
                printf("usage : gallsh -a <directory>\n");
                exit(1);
            }
        }

        else if(!strcmp(argv[i], "-m"))
            ud->maximized = true;
        else if(!strcmp(argv[i], "-h")) {
            printf("gallsh [pattern]\n\t-r // no random\n\t-m  // maximized\n");
            free(ud);
            exit(0);
        }
        else {
            pattern = strdup(argv[i]);
            break;
        }
    }
    get_image_directory(image_directory);
    ud->count = count_directory_entries(Image_Directory, pattern);
    if(ud->count == 0) {
        if(pattern)
            fprintf(stderr, "no file found in the directory %s for pattern %s\n", Image_Directory, pattern);
        else
            fprintf(stderr, "no file found in the directory %s\n", Image_Directory);
        return 1;
    }
    if(pattern)
        g_print("%d images in the directory for pattern %s\n", ud->count, pattern);
    else
        g_print("%d images in the directory\n", ud->count);

    ud->filenames = (char **)malloc(sizeof(char *) * ud->count);
    ud->times_viewed = (int *)malloc(sizeof(int) * ud->count);
    for(int i=0; i < ud->count; ud->times_viewed[i++] = 0);
    int count = read_filenames(ud->filenames, Image_Directory, pattern);
    assert(count == ud->count);
    select_random_image(ud);
    app = gtk_application_new(NULL, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), ud);
    status = g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);
    destroy_filenames(ud->filenames, ud->count);
    free(ud);
    if(pattern)
        free(pattern);
    return status;
}

