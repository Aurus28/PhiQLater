// Include things
#include "glib-object.h"
#include "glib.h"
#include "gmodule.h"
#include <gtk/gtk.h>
#include <gmp.h>


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

    // fill output with text
    gtk_label_set_label(GTK_LABEL(output), input);

    // check if this calc field was used before
    gboolean used = gtk_widget_get_visible(output);

    // make output label visible
    gtk_widget_set_visible(output, true);

    // add new calc line under if it wasnt used before (parent of parent of parent of the button that was clicked is the box where to add a new row)
    if(!used) {
        gtk_box_append(GTK_BOX(gtk_widget_get_parent(gtk_widget_get_parent(parent))), create_row());
    }
}

static void on_activate (GtkApplication *app) {
    // define things
    GtkBuilder *builder;
    GObject *window;

    // assign things
    builder = gtk_builder_new_from_file("/home/aurus28/Documents/PhiQLater/phiqlater.ui");
    window = gtk_builder_get_object(builder, "main_window");

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
    // Create a new application
    GtkApplication *app = gtk_application_new ("de.aurus28.PhiQLater", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
    return g_application_run (G_APPLICATION(app), argc, argv);
}
