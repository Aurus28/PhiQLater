// Include things
#include "glib-object.h"
#include "glib.h"
#include "gmodule.h"
#include <gtk/gtk.h>
#include <gmp.h>
#include <string.h>
#include <math.h>


GRegex *regex;


GPtrArray *parse_input(const char *input) {
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

    return array;
}

gchar check_type(const char *token) {

    switch (token[0]) {
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '0':
            return 'n';
        case '+':
            return '+';
        case '-':
            return '-';
        case '*':
            return '*';
        case '/':
            return '/';
        case '(':
            return '(';
        case ')':
            return ')';
        default:
            return 'u';
    }
}

char check_decimal(const char *token) {
    char is_decimal = 0; // 0 => normal | 'd' => decimal | 'e' => e notation | 98 => decimal & e notation | 99 => normal & decimal e notation | 100 => decimal & decimal e notation, e.g. 4.4e5.5
    for (int j = 0; token[j] != '\0'; j++) {
        if (token[j] == '.') {
            if (is_decimal == 0) {
                is_decimal = 'd';
            } else if (is_decimal == 'e') {
                is_decimal = 99;
            } else {
                is_decimal = 100;
            }
        } else if (token[j] == 'e') {
            if (is_decimal == 0) {
                is_decimal = 'e';
            } else if (is_decimal == 'd') {
                is_decimal = 98;
            }
        }
    }
    return is_decimal;
}

void mpq_set_str_fractions(mpq_t result, const char *input) {
    char *dot = strchr(input, '.');

    char *fractions = dot + 1;

    char **parts = g_strsplit(input, ".", -1);
    char *denominator = g_strjoinv("", parts);
    char *numerator = g_strdup_printf("%d", (int) pow(10, (double) strlen(fractions)));
    char *converted;


    if (input[0] == '-') {
        converted = g_strjoin("", "-", denominator, "/", numerator, nullptr);
    } else {
        converted = g_strjoin("", denominator, "/", numerator, nullptr);
    }
    mpq_set_str(result, converted, 10);

    free(parts);
    g_free(converted);
}



void mpq_pow(mpq_t result, mpq_t base, const int exp) {
    // do pow for numerator and denominator independently
    if (exp >= 0) {
        mpz_pow_ui(mpq_numref(result), mpq_numref(base), exp);
        mpz_pow_ui(mpq_denref(result), mpq_denref(base), exp);
    } else {
        // negative exponent broken
        mpz_pow_ui(mpq_numref(result), mpq_denref(base), -exp);
        mpz_pow_ui(mpq_denref(result), mpq_numref(base), -exp);
    }
    mpq_canonicalize(result);
}


gboolean mpq_set_str_e(mpq_t result, const char* input) {
    // check for decimal
    // 0 => normal | 'd' => decimal | 'e' => e notation | 98 => decimal & e notation | 99 => normal & decimal e notation | 100 => decimal & decimal e notation, e.g. 4.4e5.5
    char decimal = check_decimal(input);

    if (decimal == 0) {
        mpq_set_str(result, input, 10);
    } else if (decimal == 'd') {
        mpq_set_str_fractions(result, input);
    } else if (decimal == 'e') {
        mpq_t y, z;
        mpq_init(y);
        mpq_init(z);

        char **parts = g_strsplit(input, "e", 2);
        // set x to the int before e
        mpq_set_str(result, parts[0], 10);

        //set z to 10
        mpq_set_str(z, "10", 10);

        // set y to 10^whatever
        mpq_pow(y, z, atoi(parts[1]));

        // multiply x by y
        mpq_mul(result, result, y);
        mpq_canonicalize(result);

        free(parts);
        mpq_clear(y);
        mpq_clear(z);
    } else if (decimal == 98) {
        mpq_t y, z;
        mpq_init(y);
        mpq_init(z);

        char **parts = g_strsplit(input, "e", 2);

        // make x be the fraction before e
        mpq_set_str_fractions(result, parts[0]);

        // set z to 10
        mpq_set_str(z, "10", 10);

        // set y to 10^ whatever
        mpq_pow(y, z, atoi(parts[1]));

        // multiply x by y
        mpq_mul(result, result, y);
        mpq_canonicalize(result);

        free(parts);
        mpq_clear(y);
    } else {
        // num must be something e fraction (oftentimes irrational), no support yet
        return false;
    }
    return true;
}

gboolean interpret_input(mpq_t result, GPtrArray *tokens) {
    mpq_set_str(result, "0", 10);

    // check if there are as many '(' as ')'
    int count_brackets = 0;
    for (int i = 0; i < tokens->len -1; i++) {
        if (count_brackets < 0) return false;
        if (check_type(g_ptr_array_index(tokens, i)) == '(') count_brackets++;
        if (check_type(g_ptr_array_index(tokens, i)) == ')') count_brackets--;
    }
    if (count_brackets != 0) return false;

    gboolean done = false;
    while (true) {
        done = true;
        for (int i = 0; i < tokens->len -1; i++) {
            if (check_type(g_ptr_array_index(tokens, i)) == '(') {
                done = false;
                break;
            }
        }
        if (done) break;

        int latest_idx = 0;

        for (int i = 0; i < tokens->len -1; i++) {
            if (check_type(g_ptr_array_index(tokens, i)) == '(') latest_idx = i;
            if (check_type(g_ptr_array_index(tokens, i)) == ')') break;
        }
        g_print("index of found '(': %d\n", latest_idx);

        mpq_t x;
        mpq_init(x);
        mpq_set_str(x, "0", 10);

        gboolean partially_done = false;
        for (int i = latest_idx + 1; !partially_done; i++) {
            switch (check_type(g_ptr_array_index(tokens, i))) {
                case 'n':
                    mpq_t y;
                    mpq_init(y);

                    mpq_set_str_e(y, g_ptr_array_index(tokens, i));

                    switch (check_type(g_ptr_array_index(tokens, i -1))) {
                        case '(':
                        case '+':
                            mpq_add(x, x, y);
                            break;
                        case '-':
                            mpq_sub(x, x, y);
                            break;
                        case '*':
                            mpq_mul(x, x, y);
                            break;
                        case '/':
                            mpq_div(x, x, y);
                            break;
                        default:
                            // has to be smth wrong
                            return false;
                    }
                    mpq_canonicalize(x);
                    break;
                case '+':
                case '-':
                case '*':
                case '/':
                    break;
                case ')':
                    for (int j = i; j>= latest_idx; j--) {
                        g_ptr_array_remove_index(tokens, j);
                    }
                    g_ptr_array_insert(tokens, latest_idx, g_strdup(mpq_get_str(nullptr, 10, x)));

                    partially_done = true;
                    break;
                default:
                    return false;
            }
        }
    }

    for (int i = 0; i < tokens->len -1; i++) {
        g_print ("token: %s\n", g_ptr_array_index(tokens, i));
        switch (check_type(g_ptr_array_index(tokens, i))) {
            case 'n':
                mpq_t y;
                mpq_init(y);

                mpq_set_str_e(y, g_ptr_array_index(tokens, i));
                if (i == 0) {
                    mpq_add(result, result, y);
                    break;
                }

                switch (check_type(g_ptr_array_index(tokens, i -1))) {
                    case '+':
                        mpq_add(result, result, y);
                        break;
                    case '-':
                        mpq_sub(result, result, y);
                        break;
                    case '*':
                        mpq_mul(result, result, y);
                        break;
                    case '/':
                        mpq_div(result, result, y);
                        break;
                    default:
                        // has to be smth wrong
                        return false;
                }
                mpq_canonicalize(result);
                break;
            case '+':
            case '-':
            case '*':
            case '/':
                break;
            default:
                return false;
        }
    }
    return true;
}

// gboolean interpret_input(mpq_t result, char **tokens) {
//
//     mpq_set_str(result, "0", 10);
//
//     for (int i = 0; tokens[i] != NULL; i++) {
//         if (strcmp(check_type(tokens[i]), "unidentified") == 0) {
//             return false;
//         }
//         // if num: check prev token. then do ret (+,-,*,/) num. if not num continue if there is no prev token just add num or if prev token also num then return an error
//         if (strcmp(check_type(tokens[i]), "num") == 0) {
//
//             mpq_t x;
//             mpq_init(x);
//             if (!mpq_set_str_e(x, tokens[i])) return false;
//
//             if (i > 0) {
//
//                 if (strcmp(check_type(tokens[i-1]), "-") == 0) {
//                     mpq_sub(result, result, x);
//                     mpq_canonicalize(result);
//                     continue;
//                 }
//                 if (strcmp(check_type(tokens[i-1]), "*") == 0) {
//                     mpq_mul(result, result, x);
//                     mpq_canonicalize(result);
//                     continue;
//                 }
//                 if (strcmp(check_type(tokens[i-1]), "/") == 0) {
//                     mpq_div(result, result, x);
//                     mpq_canonicalize(result);
//                     continue;
//                 }
//                 if (strcmp(check_type(tokens[i-1]), "num") == 0) {
//                     return false;
//                 }
//             }
//             mpq_add(result, result, x);
//             mpq_canonicalize(result);
//             mpq_clear(x);
//         }
//     }
//     mpq_canonicalize(result);
//     return true;
// }

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
char *tokens_into_one(const GPtrArray *input) {
    return g_strjoinv(" ", (gchar **)input->pdata);
}


G_MODULE_EXPORT void perform_calculation(GtkWidget *widget, gpointer data) {
    //get the entry field
    GtkWidget *entry;
    if (GTK_IS_BUTTON(widget)) {
        entry = gtk_widget_get_prev_sibling(widget);
    } else {
        entry = widget;
    }

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

    GPtrArray *tokens = parse_input(input);

    // debugging
    char *tokenised_str = tokens_into_one(tokens);
    g_print("%s\n", tokenised_str);

    // fill output with text
    // have exact number
    mpq_t result;
    mpq_init(result);
    gboolean worked =  interpret_input(result, tokens);

    if (worked) {
        // now make it String
        mpq_canonicalize(result);
        char *str = mpq_get_str(nullptr, 10, result);

        // set output
        gtk_label_set_label(GTK_LABEL(output), str);

        // freeeee
        free(str);
    } else {
        const char *error = strcat(tokenised_str, "couldn't be interpreted");
        gtk_label_set_label(GTK_LABEL(output), error);
    }
    // check if this calc field was used before
    gboolean used = gtk_widget_get_visible(output);

    // make output label visible
    gtk_widget_set_visible(output, true);

    // add new calc line under if it wasn't used before (parent of parent of parent of the button that was clicked is the box where to add a new row)
    if(!used) {
        gtk_box_append(GTK_BOX(gtk_widget_get_parent(gtk_widget_get_parent(parent))), create_row());
    }

    // free memory
    g_print("freeing...\n");
    g_ptr_array_free(tokens, TRUE);
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
