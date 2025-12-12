// Include things
#include "glib-object.h"
#include "glib.h"
#include "gmodule.h"
#include <gtk/gtk.h>
#include <gmp.h>
#include <string.h>


GRegex *regex;


gchar **parse_input(const char *input) {
    // prepare info variable
    GMatchInfo *match_info;

    // match input to regex
    g_regex_match(regex, input, 0, &match_info);

    GPtrArray *array = g_ptr_array_new_with_free_func(g_free);

    while (g_match_info_matches(match_info)) {
        gchar *token = g_match_info_fetch(match_info, 0);
        g_ptr_array_add(array, token);
        g_match_info_next(match_info, nullptr);
    }

    g_ptr_array_add(array, NULL);

    // free thingos
    g_match_info_free(match_info);

    return (gchar**) g_ptr_array_free(array, FALSE);
}



gchar *interpret_input(const char **tokens) {

    GPtrArray *array = g_ptr_array_new_with_free_func(g_free);

    for (int i = 0; tokens[i] != NULL; i++) {
        switch (tokens[i][0]) {
            case '1' || '2' || '3' || '4' || '5' || '6' || '7' || '8' || '9' || '0' :
                mpq_t x;
                mpq_init(x);
                mpq_set_str(x, tokens[i], 10);
                gmp_printf("%Qd\n", x);
                g_ptr_array_add(array, x);
                break;
            case '+':
                // if next token is a num do calculation or something, idk
                break;
            case '-':
                break;
            case '*':
                break;
            case '/':
                break;
            default:

        }
    }
    return ":)";
}

GtkWidget *create_row() {
    // get widget from file
    GtkBuilder *builder = gtk_builder_new_from_file("/home/aurus28/Documents/PhiQLater/calc_row.ui");
    GtkWidget *row = GTK_WIDGET(gtk_builder_get_object(builder, "row_root"));

    // keep widget alive
    g_object_ref(row);

    // unref builder
    g_object_unref(builder);

    // return new row
    return row;
}

//sidequest
char *tokens_into_one(gchar **input) {
    GString *builder = g_string_new(nullptr);

    for (int i = 0; input[i] != NULL; i++) {
        g_string_append(builder, input[i]);
        g_string_append(builder, " | ");
    }

    return g_string_free(builder, FALSE); // returns char*, do NOT free builder
}


G_MODULE_EXPORT void perform_calculation(GtkWidget *widget, gpointer data) {
    //get the entry field
    GtkWidget *entry = gtk_widget_get_prev_sibling(widget);

    // get the user input
    const char *input = gtk_editable_get_text(GTK_EDITABLE(entry));

    // check if there is any input
    if(!strcmp(input, "")) {
        return;
    }

    // print user input for debugging
    //g_print("%s\n", input);

    //get parent & parent sibling (output) element
    GtkWidget *parent = gtk_widget_get_parent(widget);
    GtkWidget *output = gtk_widget_get_next_sibling(parent);

    gchar **tokens = parse_input(input);
    char *tokenised_str = tokens_into_one(tokens);

    // fill output with text
    gtk_label_set_label(GTK_LABEL(output), tokenised_str);

    // check if this calc field was used before
    gboolean used = gtk_widget_get_visible(output);

    // make output label visible
    gtk_widget_set_visible(output, true);

    // add new calc line under if it wasn't used before (parent of parent of parent of the button that was clicked is the box where to add a new row)
    if(!used) {
        gtk_box_append(GTK_BOX(gtk_widget_get_parent(gtk_widget_get_parent(parent))), create_row());
    }

    // free memory
    g_strfreev(tokens);
    g_free(tokenised_str);
}

static void on_activate (GtkApplication *app) {

    // assign things
    GtkBuilder *builder = gtk_builder_new_from_file("/home/aurus28/Documents/PhiQLater/phiqlater.ui");
    GObject *window = gtk_builder_get_object(builder, "main_window");

    // error handling
    if(!window) {
        g_error("Failed to find \"main_window\" in \"phiqlater.ui\"!");
    }

    //assign window to application
    gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));

    // show window
    gtk_window_present(GTK_WINDOW(window));

    g_object_unref(builder);
}

int main (int argc, char *argv[]) {
    // regex (for later use)
    regex = g_regex_new(
    "[0-9]+(?:\\.[0-9]+)?+(?:e+(?:-)?[0-9]+)?"   // numbers (with e)
    "|[A-Za-z]+"             // identifiers (sqrt, sin, ...)
    "|\\*|/|\\+|-"            // operators
    "|\\(|\\)",              // parentheses
    G_REGEX_OPTIMIZE,
    0,
    nullptr
    );


    // Create a new application
    GtkApplication *app = gtk_application_new ("de.aurus28.PhiQLater", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);

    int ret = g_application_run (G_APPLICATION(app), argc, argv);
    
    // free memory
    g_regex_unref(regex);
    g_object_unref(app);
    
    return ret;
}
