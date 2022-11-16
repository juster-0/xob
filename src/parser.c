/* xob - A lightweight overlay volume/anything bar for the X Window System.
 * Copyright (C) 2022 Smoliarchuk Illia
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

#include "parser.h"
#include "log.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void dyn_str_add_node(Dynamic_string *pdyn_str,
                             Dynamic_string_node *pdyn_node);

Dynamic_string generate_dyn_str(const char *str)
{
    Dynamic_string dyn_str;
    Dynamic_string_node *dyn_str_node = NULL;
    char *start;
    char *end = strchr(str, '\0');
    const char *lead = str;
    int index;
    int loop = 0;
    char buffer[strlen(str) + 1];
    int buffer_size = 0;
    buffer[0] = '\0';

    dyn_str.inserts = 0;
    dyn_str.first = NULL;

    while (loop == 0)
    {
        if (!dyn_str_node)
        {
            dyn_str_node =
                (Dynamic_string_node *)malloc(sizeof(Dynamic_string_node));
            print_loge_once("DEBUG: malloc memory for Dynamic_string_node\n");
            dyn_str_node->next = NULL;
            dyn_str_node->str = NULL;
        }

        start = strchr(lead, '{'); // Look for the first '{'
        if (start == NULL)         // if no '{' found so finish parsing
        {
            loop = 1;
            break;
        }
        else if (end - start > 1)
        {
            /* Define if the next symbol after '{' is digit */
            if (sscanf(&start[1], "%d", &index))
            {
                /* Get index length */
                int i, index_length;
                for (i = index / 10, index_length = 1; i > 0;
                     i /= 10, index_length++)
                    continue;
                print_loge("DEBUG: index_length is [%d]\n", index_length);

                /* Check if '}' exists after number */
                if (start[index_length + 1] != '}')
                {
                    loop = 1;
                    break;
                }
                /* syntax is correct, add part before '{' to dyn_str and add
                 * index to a node with str */

                dyn_str_node->index = index;

                /* Copy text before { to the buffer */
                strncat(buffer, lead, start - lead);
                buffer[buffer_size + start - lead + 1] = '\0';
                buffer_size += start - lead;

                lead = start + index_length + 2;

                /* Malloc memory for str */
                dyn_str_node->str =
                    (char *)malloc(sizeof(char) + strlen(buffer) + 1);
                strncpy(dyn_str_node->str, buffer, strlen(buffer) + 1);

                /* Make buffer "empty" */
                buffer[0] = '\0';
                buffer_size = 0;

                /* Add node to dyn_str */
                dyn_str_add_node(&dyn_str, dyn_str_node);
                dyn_str_node = NULL;

                continue;
            }
            else if (start[1] == '{') // '{{' is '{' in str
            {
                /* Copy content before the first '{' */
                strncat(buffer, lead, start - lead);
                /* Set \0 to buffer after previous str */
                buffer[buffer_size + start - lead + 1] = '\0';
                buffer_size += start - lead;
                buffer[buffer_size - 0] = '{';

                lead = start + 2;
                continue;
            }
            else
            {
                fprintf(stderr, "Error: Not correct syntax, ignore the end of"
                                " the string.\n");
                loop = 1;
                break;
            }
        }
        else
        {
            fprintf(stderr, "Error: something goes wrong.\n");
        }
    }

    switch (loop)
    {
    case 0:
        break;
    case 1:
        print_loge_once("DEBUG: get last line of dyn_str\n");
        strncat(buffer, lead, end - lead);
        buffer[buffer_size + end - lead + 1] = '\0';
        buffer_size += end - lead;

        /* Malloc and copy buffer to dyn_str_node */
        dyn_str_node->str = (char *)malloc(sizeof(char) * strlen(buffer) + 1);
        strncpy(dyn_str_node->str, buffer, strlen(buffer) + 1);

        buffer[0] = '\0';
        buffer_size = 0;

        dyn_str_add_node(&dyn_str, dyn_str_node);

        break;
    default:
        break;
    }

    /* Count length of dynamic_str */
    int dyn_str_len = 0;
    dyn_str_node = dyn_str.first;
    while (dyn_str_node)
    {
        dyn_str_len += strlen(dyn_str_node->str);
        dyn_str_node = dyn_str_node->next;
    }
    dyn_str.str_len = dyn_str_len;
    print_loge("DEBUG: dyn_str_len is [%d]\n", dyn_str_len);

    return dyn_str;
}

void free_dyn_str(Dynamic_string *pdyn_str)
{
    Dynamic_string_node *dyn_str_node, *dyn_str_node_prev;

    dyn_str_node_prev = NULL;
    dyn_str_node = pdyn_str->first;
    while (dyn_str_node)
    {
        free(dyn_str_node->str);
        free(dyn_str_node_prev);

        dyn_str_node_prev = dyn_str_node;
        dyn_str_node = dyn_str_node->next;
    }
    free(dyn_str_node_prev);

    pdyn_str->first = 0;
    pdyn_str->inserts = 0;
    pdyn_str->str_len = 0;
}

int strlen_dyn_str(const Dynamic_string *pdyn_str) { return pdyn_str->str_len; }

int fill_dyn_str(char *str, Dynamic_string *pdyn_str, char **words_list,
                 int words_list_len)
{
    int status = 0;
    str[0] = '\0';

    /* Fill dynamic string */
    Dynamic_string_node *dyn_str_node = pdyn_str->first;
    while (dyn_str_node->next)
    {
        if (dyn_str_node->index >= words_list_len)
            return 0;
        strcat(str, dyn_str_node->str);
        strcat(str, words_list[dyn_str_node->index]);
        status++;
        dyn_str_node = dyn_str_node->next;
    }
    strcat(str, dyn_str_node->str);

    return status;
}

char *parse_splitted(char *str)
{
    static char *input;
    char *lead;
    bool in_block = false;
    uint8_t block_index;
    char *p_block_index;
    char *block_chars = "\"'";
    size_t string_len;

    if (str != NULL)
    {
        input = str;
        lead = input;
    }
    else
    {
        if (input[0] == '\0')
            return NULL;
        lead = input;
    }
    while (input[0] != '\0')
    {
        if (in_block)
        {
            // printf("input is [%s]\n", input);
            // printf("in_block block_index[%d]\n", block_index);
            p_block_index = strchr(input, block_chars[block_index]);
            // printf("p_block_index [%s]\n", p_block_index);
            in_block = false;
            if (p_block_index)
            {
                // printf("found end\n");
                // printf("p_block_index[block_index] is [%c]\n",
                // block_chars[block_index]); printf("p_block_index[%p]\n",
                // p_block_index);
                input = p_block_index + 1;
                // printf("input [%s]\n", input);
                input[-1] = '\0';
                // in_block = 0;
                break;
                // continue;
            }
            // else
            // {
            //     in_block = 0;
            // }
        }
        else if ((p_block_index = strchr(block_chars, input[0])) != NULL)
        {
            // printf("input[0] is [%c]\n", input[0]);
            // printf("p_block_index[%p]\n", p_block_index);
            block_index = p_block_index - block_chars;
            // printf("block_index[%d]\n", block_index);
            // printf("lead [%s]\n", lead);
            in_block = true;
            lead++;
            input++;
            continue;
        }

        /* Allow escape special symbols with \ */
        if (input[0] == '\\')
        {
            switch (input[1])
            {
            case '\\':
            case ' ':
            case '\'':
            case '"':
                string_len = strlen(input + 1);
                memmove(input, input + 1, string_len + 1);
                input++;
                continue;
                break;
            case 'n':
            case 't':
                fprintf(stderr, "Warning: \\n and \\t are not supported by"
                                " xob, don't use them (we may add them in the"
                                " future\n");
                input += 2;
                continue;
            case '\0':
                fprintf(stderr, "Warning: no specifier found after \\\n");
                input++;
                continue;
                break;
            default:
                fprintf(stderr,
                        "Warning: \\%c is not correct escape"
                        " sequece, don't use them\n",
                        input[1]);
                input += 2;
                continue;
                break;
            }
        }

        if (input[0] == ' ')
        {
            if (input - lead == 0)
            {
                lead++;
                input++;
                continue;
            }

            if (input[0] != '\0')
            {
                input[0] = '\0';
                input++;
            }
            break;
        }
        input++;
    }
    return lead;
}

static void dyn_str_add_node(Dynamic_string *pdyn_str,
                             Dynamic_string_node *pdyn_node)
{
    print_loge_once("DEBUG: add node to dyn_str\n");
    Dynamic_string_node *dyn_str_node;
    if (!pdyn_str->first)
    {
        pdyn_str->first = pdyn_node;
        pdyn_str->inserts = 1;
        return;
    }
    else
    {
        dyn_str_node = pdyn_str->first;
    }

    while (dyn_str_node->next)
        dyn_str_node = dyn_str_node->next;

    dyn_str_node->next = pdyn_node;
    pdyn_str->inserts++;

    return;
}
