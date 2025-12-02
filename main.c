// Include gtk
#include <gtk/gtk.h>

static void on_activate (GtkApplication *app) {
    // Create a new window
    GtkWidget *window = gtk_application_window_new(app);
    
    

    // title
    gtk_window_set_title(GTK_WINDOW(window), "PhiQLater");

    //size
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 400);

    // Create a new button
    GtkWidget *button = gtk_button_new_with_label("Close Window");
    gtk_widget_set_halign(button, GTK_ALIGN_END + 10);
    gtk_widget_set_valign(button, GTK_ALIGN_END + 10);
    
    // When the button is clicked, close the window passed as an argument
    g_signal_connect_swapped (button, "clicked", G_CALLBACK(gtk_window_close), window);
    gtk_window_set_child (GTK_WINDOW(window), button);
    gtk_window_present (GTK_WINDOW(window));
}

int main (int argc, char *argv[]) {
    // Create a new application
    GtkApplication *app = gtk_application_new ("de.aurus28.PhiQLater", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
    return g_application_run (G_APPLICATION (app), argc, argv);
}
