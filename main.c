// Include things
#include "glib-object.h"
#include "glib.h"
#include "gmodule.h"
#include <gtk/gtk.h>
#include <gmp.h>
#include <string.h>
#include <math.h>


GRegex *regex;
typedef struct Token {
    char type; // n: number | u: undefined | +-*/(): that sign | to be continued
    char *value;
    char owner; // u: user (by entering his input) | o: order of operations
} Token;

void free_token(Token *t) {
    g_free(t->value);
    g_free(t);
}

GPtrArray *tokenize(const char *input) {
    // prepare info variable
    GMatchInfo *match_info;

    // match input to regex
    g_regex_match(regex, input, 0, &match_info);

    GPtrArray *array = g_ptr_array_new_with_free_func((GDestroyNotify)free_token);

    while (g_match_info_matches(match_info)) {
        Token *t = g_new(Token, 1);
        t->type = 'u';
        t->owner = 'u';
        t->value = g_match_info_fetch(match_info, 0);
        g_ptr_array_add(array, t);
        g_match_info_next(match_info, nullptr);
    }

    // free thingos
    g_match_info_free(match_info);

    return array;
}

char *tokens_into_one(const GPtrArray *input) {
    GString *builder = g_string_new(nullptr);

    for (guint i = 0; i < input->len; i++) {
        const Token *t = g_ptr_array_index(input, i);

        g_string_append(builder, t->value);
        g_string_append_c(builder, ' ');
    }

    return g_string_free(builder, FALSE); // caller must g_free()
}

void set_types(const GPtrArray *tokens) {
    for (int i = 0; i < tokens->len; i++) {
        Token *t = g_ptr_array_index(tokens, i);
        switch (t->value[0]) {
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
                t->type = 'n';
                break;
            case '+':
            case '-':
            case '*':
            case '/':
            case '(':
            case ')':
                t->type = t->value[0];
                break;
            default:
                t->type = 'u';
                break;
        }
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


gboolean mpq_set_str_fractions_and_e(mpq_t result, const char* input) {
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

// only works if tokens are only numbers and basic operations!!!
int combine_tokens_to_mpq(int start, const GPtrArray *tokens, mpq_t result) {
    for (int i = start; i < tokens->len; i++) {

        const Token *t1 = g_ptr_array_index(tokens, i);

        switch (t1->type) {
            case 'n':
                mpq_t y;
                mpq_init(y);

                mpq_set_str_fractions_and_e(y, t1->value);

                if (i > 0) {
                    const Token *t2 = g_ptr_array_index(tokens, i-1);
                    switch (t2->type) {
                        case '(':
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
                } else {
                    // it's the first number of the statement, so just add it
                    mpq_add(result, result, y);
                }
                mpq_canonicalize(result);
                break;
            case '+':
            case '-':
            case '*':
            case '/':
                break;
            case '(':
            case ')':
            default:
                return i;
        }
    }
    return true;
}

gboolean add_brackets_for_order(GPtrArray *tokens) {
    GArray *plusminus_indices = g_array_new(FALSE, FALSE, sizeof(int));
    g_array_append_val(plusminus_indices, (int){0});

    for (int i = 0; i < tokens->len; i++) {
        Token *t1 = g_ptr_array_index(tokens, i);
        switch (t1->type) {
            case 'n':
                break;
            case '+':
            case '-':
                g_array_index(plusminus_indices, int, plusminus_indices->len -1) = i;
                break;
            case '*':
            case '/':
                // add '(' right after the plus or minus
                int current_idx = g_array_index(plusminus_indices, int, plusminus_indices->len -1);
                if (current_idx != 0) {
                    Token *t2 = g_new(Token, 1);
                    t2->type = '(';
                    t2->value = strdup("(");
                    t2->owner = 'o';
                    g_ptr_array_insert(tokens, current_idx +1, t2);

                    // debug
                    char *tokenised_str = tokens_into_one(tokens);
                    g_print("%s\n", tokenised_str);

                    // find spot to put ')'
                    int x = 0;
                    // start at i+1 because I added something above
                    for (int j = i + 1; j < tokens->len; j++) {
                        Token *t3 = g_ptr_array_index(tokens, j);
                        if (t3->type == '(') {
                            x++;
                        } else if (t3->type == ')') {
                            x--;
                        } else if (t3->type == 'n') {
                            if (x == 0) {
                                Token *t4 = g_new(Token, 1);
                                t4->type = ')';
                                t4->owner = 'o';
                                t4->value = g_strdup(")");
                                g_ptr_array_insert(tokens, j + 1, t4);

                                tokenised_str = tokens_into_one(tokens);
                                g_print("%s\n", tokenised_str);

                                g_array_index(plusminus_indices, int, plusminus_indices->len -1) = 0;
                                break;
                            }

                        }
                    }
                    g_free(tokenised_str);
                    if (x != 0) return false;
                }
                break;
            case '(':
                g_array_append_val(plusminus_indices, (int){0});
                break;
            case ')':
                // if check to avoid removing a index that was never there
                if (t1->owner == 'u') {
                    g_array_remove_index(plusminus_indices, plusminus_indices->len - 1);
                }
                break;
            default:
                return false;
        }
    }
    return true;
}

gboolean interpret_input(mpq_t result, GPtrArray *tokens) {
    mpq_set_str(result, "0", 10);

    set_types(tokens);

    if (!add_brackets_for_order(tokens)) return false;

    // check if there are as many '(' as ')'
    int count_brackets = 0;
    for (int i = 0; i < tokens->len; i++) {
        if (count_brackets < 0) return false;
        const Token *t = g_ptr_array_index(tokens, i);
        if (t->type == '(') count_brackets++;
        if (t->type == ')') count_brackets--;
    }
    if (count_brackets != 0) return false;

    gboolean done = false;
    while (true) {

        // check if there are brackets left
        done = true;
        for (int i = 0; i < tokens->len -1; i++) {
            const Token *t = g_ptr_array_index(tokens, i);
            if (t->type == '(') {
                done = false;
                break;
            }
        }
        if (done) break;


        // find index of innermost opening bracket
        int latest_idx = 0;

        for (int i = 0; i < tokens->len -1; i++) {
            const Token *t = g_ptr_array_index(tokens, i);
            if (t->type == '(') latest_idx = i;
            if (t->type == ')') break;
        }
        g_print("index of found '(': %d\n", latest_idx);

        // temporary mpq_t for combining
        mpq_t x;
        mpq_init(x);
        mpq_set_str(x, "0", 10);

        // combine all tokens up to ')' into mpq_t x
        int exit_at = combine_tokens_to_mpq(latest_idx + 1, tokens, x);

        // check if it exited prior due to unknown symbols or later due to missing ')'
        Token *exit_token = g_ptr_array_index(tokens, exit_at);
        if (exit_token->type == ')') {
            // remove the brackets expression from tokens and add the result of it in its place
            for (int j = exit_at; j>= latest_idx; j--) {
                g_ptr_array_remove_index(tokens, j);
            }
            // build new token to insert
            Token *new_token = g_new(Token, 1);
            new_token->type = 'n';
            new_token->value = g_strdup(mpq_get_str(nullptr, 10, x));
            g_ptr_array_insert(tokens, latest_idx, new_token);
        } else {
            // something has to be wrong
            return false;
        }
    }
    g_print("%s\n", tokens_into_one(tokens));

    // all the brackets are gone so now just do the rest
    if (combine_tokens_to_mpq(0, tokens, result) == false) return false;
    return true;
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

    GPtrArray *tokens = tokenize(input);

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
