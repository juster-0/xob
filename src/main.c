/* xob - A lightweight overlay volume/anything bar for the X Window System.
 * Copyright (C) 2021 Florent Ch.
 *
 * xob is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * xob is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xob.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _XOPEN_SOURCE 500
#include "main.h"
#include "conf.h"
#include "display.h"
#include "log.h"
#include "parser.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int cap = 100;
    int timeout = 1000;

    char *arg_config_file_path = NULL;
    char *style_name = DEFAULT_STYLE;

    /* Command-line arguments */
    int opt;
    while ((opt = getopt(argc, argv, "m:t:c:s:qvh")) != -1)
    {
        switch (opt)
        {
        case 'm':
            cap = atoi(optarg);
            if (cap <= 0)
            {
                fprintf(
                    stderr,
                    "Invalid cap (maximum value): must be a natural number.\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 't':
            timeout = atoi(optarg);
            if (timeout < 0)
            {
                fprintf(stderr, "Invalid timeout: must be a natural number.\n");
                exit(EXIT_FAILURE);
            }
            else if (timeout == 0)
            {
                fprintf(stderr, "Info: no timeout, the bar will "
                                "remain on-screen.\n");
            }
            else if (timeout < 100)
            {
                fprintf(
                    stderr,
                    "Warning: timeout is low, the bar may not be visible.\n");
            }
            break;
        case 'c':
            arg_config_file_path = optarg;
            break;
        case 's':
            style_name = optarg;
            break;
        case 'q':
            freopen("/dev/null", "w", stdout);
            break;
        case 'v':
            printf("Version %s\n", VERSION_NUMBER);
            exit(EXIT_SUCCESS);
            break;
        default:
            fprintf(stderr,
                    "Usage: %s [-m maximum] [-t timeout] [-c configfile] [-s "
                    "style]\n\n",
                    argv[0]);
            fprintf(stderr, "    -m <non-zero natural>"
                            " maximum value (0 is always the minimum)\n");
            fprintf(stderr,
                    "    -t <natural>         "
                    " duration in milliseconds between an update and the "
                    "vanishing of the bar "
                    "after an update or 0 if always on screen\n");
            fprintf(stderr, "    -c <filepath>        "
                            " configuration file specifying styles\n");
            fprintf(stderr, "    -s <style name>      "
                            " style to use from the configuration file\n");
            fprintf(stderr, "    -q                   "
                            " suppress all normal output\n");
            fprintf(stderr, "    -v                   "
                            " display version number\n");
            exit(EXIT_FAILURE);
        }
    }

    /* Style */
    FILE *config_file = NULL;
    Style style = DEFAULT_CONFIGURATION;
    char xdg_config_file_path[PATH_MAX];
    char real_config_file_path[PATH_MAX];

    /* Case #1: config file in argument */
    if (arg_config_file_path != NULL)
    {
        if (realpath(arg_config_file_path, real_config_file_path) != NULL)
        {
            config_file = fopen(real_config_file_path, "r");
        }
        else
        {
            fprintf(stderr,
                    "Error: could not open specified configuration file.\n");
            fprintf(stderr,
                    "Info: falling back to standard configuration files.\n");
        }
    }

    /* Case #2: the XDG_CONFIG_HOME environment variable is set */
    if (config_file == NULL && getenv("XDG_CONFIG_HOME") != NULL)
    {
        if (snprintf(xdg_config_file_path, PATH_MAX, "%s/%s/%s",
                     getenv("XDG_CONFIG_HOME"), DEFAULT_CONFIG_APPNAME,
                     DEFAULT_CONFIG_FILENAME) < PATH_MAX)
        {
            if (realpath(xdg_config_file_path, real_config_file_path) != NULL)
            {
                config_file = fopen(real_config_file_path, "r");
            }
        }
    }

    /* Case #3: falling back to default configuration directory */
    if (config_file == NULL)
    {
        if (snprintf(xdg_config_file_path, PATH_MAX, "%s/.config/%s/%s",
                     getenv("HOME"), DEFAULT_CONFIG_APPNAME,
                     DEFAULT_CONFIG_FILENAME) < PATH_MAX)
        {
            if (realpath(xdg_config_file_path, real_config_file_path) != NULL)
            {
                config_file = fopen(real_config_file_path, "r");
            }
        }
    }

    /* Case #4: system wide configuration */
    if (config_file == NULL)
    {
        if (realpath(SYSCONFDIR "/" DEFAULT_CONFIG_APPNAME
                                "/" DEFAULT_CONFIG_FILENAME,
                     real_config_file_path) != NULL)
        {
            config_file = fopen(real_config_file_path, "r");
        }
    }

    /* Parsing the config file */
    printf("Info: reading configuration from %s.\n", real_config_file_path);
    style = parse_style_config(config_file, style_name, style);
    fclose(config_file);

    /* Display */
    bool displayed = false;
    bool listening = true;
    Input_value input_value;
    Display_context display_context = init(style);

    style_free(&style);

    char *words_list[MAX_DYN_STR_SIZE + 1]; // TODO dymamic list length

    if (display_context.x.display == NULL)
    {
        fprintf(stderr, "Error: Cannot open display\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        fd_set fds;
        struct timeval tv;

        /* Main loop */
        while (listening)
        {
            /* Waiting for input on stdin or time to hide the gauge */
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            tv.tv_sec = timeout / 1000;
            tv.tv_usec = 1000 * (timeout % 1000);
            switch (select(1, &fds, NULL, NULL,
                           /* No timeout if already hidden */
                           displayed && timeout > 0 ? &tv : NULL))
            {
            case -1:
                print_loge_once("DEBUG: select error\n");
                perror("select()");
                exit(EXIT_FAILURE);
            case 0:
                /* Time to hide the gauge */
                print_loge_once("DEBUG: select timeout, hide the bar\n");
                hide(&display_context);
                displayed = false;
                break;
            default:
                /* Update display using new input value */
                input_value = parse_input(words_list, MAX_DYN_STR_SIZE + 1);
                if (input_value.valid)
                {
                    show(&display_context, input_value.value, cap,
                         style.overflow, input_value.show_mode, words_list);
                    printf("Update: %d/%d %s\n", input_value.value, cap,
                           (input_value.show_mode == ALTERNATIVE) ? "[ALT]"
                                                                  : "");
                    displayed = true;
                    listening = true;
                }
                else
                {
                    /* Stop after unexpected input */
                    struct timespec wait_time = {timeout / 1000,
                                                 1000 * (timeout % 1000)};
                    nanosleep(&wait_time, NULL); // Waiting for timeout
                    hide(&display_context);
                    listening = false;
                }
                free_input_value(&input_value);
                break;
            }
        }

        /* Clean the memory */
        display_context_destroy(&display_context);
    }
    return EXIT_SUCCESS;
}

Input_value parse_input(char **words_list, int size)
{
    print_loge_once("DEBUG: parse_input()\n");
    Input_value input_value;
    char altflag;

    // char input_string[200];     // TODO dynamic length
    // TODO change malloc to something more simple
    input_value.input_string = (char *)malloc(sizeof(char) * 200);
    char *inp_word;
    int word_index;
    int num_len, temp_num;

    input_value.valid = false;

    /* Get input */
    char *read_status = fgets(input_value.input_string, 200, stdin);
    if (read_status == NULL)
    {
        print_loge_once("DEBUG: read_status is NULL\n");
        return input_value;
    }
    else
    {
        print_loge_once("DEBUG: read_status is not NULL\n");
    }
    input_value.input_string[strlen(input_value.input_string) - 1] = '\0';
    print_loge("DEBUG: input_value.input_string is [%s]\n",
               input_value.input_string);

    /* Split line by tokens */
    if (strlen(input_value.input_string) > 0)
    {
        inp_word = parse_splitted(input_value.input_string);
        words_list[0] = inp_word;
        print_loge("DEBUG: input_string[0] is [%s]\n", words_list[0]);
    }
    else
    {
        return input_value;
    }

    for (word_index = 1; word_index < size - 1; word_index++)
    {
        words_list[word_index] = parse_splitted(NULL);
        if (words_list[word_index] == NULL)
            break;
        print_loge("DEBUG: input_string[%d] is [%s]\n", word_index,
                   words_list[word_index]);
    }

    if (sscanf(words_list[0], "%d", &(input_value.value)) > 0)
    {
        // checking for the "alternative mode"
        input_value.show_mode = NORMAL;

        /* Calculate input_value.value length */
        temp_num = input_value.value;
        num_len = 0;
        while (temp_num > 0)
        {
            num_len++;
            temp_num /= 10;
        }

        /* Checking for the "alternative mode" flag : '!' */
        if (sscanf(words_list[0] + num_len, "%c", &altflag) > 0 &&
            altflag == '!')
        {
            print_loge("DEBUG: Input_value parse_input altflag is '%c'\n",
                       altflag);
            input_value.show_mode = ALTERNATIVE;
        }
        else
        {
            // print_loge("DEBUG: Input_value parse_input altflag is NULL\n");
            input_value.show_mode = NORMAL;
        }

        input_value.valid = true;
    }

    return input_value;
}

void free_input_value(Input_value *p_input_value)
{
    free(p_input_value->input_string);
}
