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

gchar *check_type(const char *token) {

    switch (token[0]) {
        case '1':
            return "num";
        case '2':
            return "num";
        case '3':
            return "num";
        case '4':
            return "num";
        case '5':
            return "num";
        case '6':
            return "num";
        case '7':
            return "num";
        case '8':
            return "num";
        case '9':
            return "num";
        case '0':
            return "num";
        case '+':
            return "+";
        case '-':
            return "-";
        case '*':
            return "*";
        case '/':
            return "/";
        default:
            return "unidentified";
    }
}


// currently broken
void interpret_input(mpq_t result, char **tokens) {

    mpq_set_str(result, "0", 10);

    for (int i = 0; tokens[i] != NULL; i++) {
        // if num: check prev token. then do ret (+,-,*,/) num. if not num continue if there is no prev token just add num
        if (strcmp(check_type(tokens[i]), "num") == 0) {
            if (i > 0) {
                if (strcmp(check_type(tokens[i-1]), "-") == 0) {
                    mpq_t x;
                    mpq_init(x);
                    mpq_set_str(x, tokens[i], 10);
                    mpq_sub(result, result, x);
                    continue;
                }
                if (strcmp(check_type(tokens[i-1]), "*") == 0) {
                    mpq_t x;
                    mpq_init(x);
                    mpq_set_str(x, tokens[i], 10);
                    mpq_mul(result, result, x);
                    continue;
                }
                if (strcmp(check_type(tokens[i-1]), "/") == 0) {
                    mpq_t x;
                    mpq_init(x);
                    mpq_set_str(x, tokens[i], 10);
                    mpq_div(result, result, x);
                    continue;
                }
            }
            mpq_t x;
            mpq_init(x);
            mpq_set_str(x, tokens[i], 10);
            mpq_add(result, result, x);
        }
    }
    mpq_canonicalize(result);
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

    // debugging
    char *tokenised_str = tokens_into_one(tokens);
    g_print("%s\n", tokenised_str);

    // fill output with text
    // have exact number
    mpq_t result;
    mpq_init(result);
    interpret_input(result, tokens);

    // now make it String
    char *str = mpq_get_str(nullptr, 10, result);

    // set output
    gtk_label_set_label(GTK_LABEL(output), str);

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
    free(str);
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
