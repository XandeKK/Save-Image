#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif

#include <stdio.h>
#include <string.h>// #include <stdlib.h>
#include <glob.h>
#include <regex.h>

struct Text {
    char value[256];
};

struct Texts {
    int length;
    struct Text text[30];
};

char **get_files(const char *input_dir) {
    char **found;
    glob_t gstruct;
    int r;

    r = glob(input_dir, GLOB_ERR , NULL, &gstruct);
    /* check for errors */
    if( r!=0 )
    {
        if( r==GLOB_NOMATCH )
            fprintf(stderr,"No matches\n");
        else
            fprintf(stderr,"Some kinda glob error\n");
        // exit(1);
    }
    
    /* success, output found filenames */
    printf("Found %zu filename matches\n",gstruct.gl_pathc);
    found = gstruct.gl_pathv;

    return found;
}

#include <regex.h>        
#include <stdio.h>
#include <stdlib.h>

int is_page(char *text, int page_number) {
    regex_t reegex;
    int value;

    value = regcomp( &reegex, "^##", 0 );
    value = regexec( &reegex, text, 0, NULL, 0 );

    if (value == 0) {
        char *ptr = text;
        while (*ptr && (*ptr < '0' || *ptr > '9')) {
            ptr++;
        }
        if (atoi(ptr) == page_number) {
            return 1;
        }
    }
    return 0;
}

int is_space(char *text) {
    regex_t reegex;
    int value;
    value = regcomp( &reegex, "^[ \t\n\r\f\v]*$", 0 );

    value = regexec( &reegex, text,0, NULL, 0 );

    if (value == 0) {
        return TRUE;
    }
    return FALSE;
}

// int main()
// {
//     FILE* file_ptr;
//     char str[256]; // Estou em dúvida se deixo static ou não
//     struct Texts texts = {0};
//     // char *output;
//     int status = FALSE;
//     file_ptr = fopen("/home/xandekk/Obras/Sekai_de_Tadahitori_no_Mamono_Tsukai/39/Tradução", "r");

//     if (NULL == file_ptr) {
//         char error[100];
//         snprintf(error, sizeof error, "unable to access file - %s (%d)", __FILE__ ,__LINE__);
//         // gimp_message(error);
//         // gimp_quit();
//     }

//     while (fgets(str, 256, file_ptr) != NULL) {
//         if (is_space(str)) {
//             continue;
//         }
//         if (is_page(str, 1)) {
//             status = TRUE;
//             continue;
//         } else if (is_page(str, 1 + 1)) {
//             status = FALSE;
//             break;
//         }

//         if (status) {
//             strcpy(texts.text[texts.length].value, str);
//             texts.length++;
//         }
//     }

//     for (int i = 0; i < texts.length; ++i) {
//         printf("%s\n", texts.text[i].value);
//     }

//     fclose(file_ptr);
// }

#include <stdio.h>

const char *get_biggest_word(const char *text) {
    int size = strlen(text);
    int i, len, max = -1;
    static char longestWord[50];

    for(i = 0; i <= size; i++) {
        if(text[i] == ' ' || text[i] == '\0') {
      if(len > max) {
                max = len;
        strncpy(longestWord, &text[i - len], len);
      }
      len = 0;
    } else {
            len++;
    }
  }

    return longestWord;
}

// struct Point {
//     int x;
//     int y;
// };

// void move_point(struct Point *p, int dx, int dy)
// {
//     p->x += dx;
//     p->y += dy;
// }

#include <stdio.h>
#include <string.h>

int is_valid_utf8(unsigned char *str, size_t len)
{
    size_t i = 0;
    while (i < len)
    {
        size_t sequence_length;
        if (str[i] <= 0x7F)
            sequence_length = 1;
        else if ((str[i] & 0xE0) == 0xC0)
            sequence_length = 2;
        else if ((str[i] & 0xF0) == 0xE0)
            sequence_length = 3;
        else if ((str[i] & 0xF8) == 0xF0)
            sequence_length = 4;
        else
            return 0;

        if (i + sequence_length > len)
            return 0;

        for (size_t j = 1; j < sequence_length; j++)
            if ((str[i + j] & 0xC0) != 0x80)
                return 0;

        i += sequence_length;
    }

    return 1;
}

void remove_invalid_utf8(unsigned char *str, size_t len)
{
    size_t i = 0;
    while (i < len)
    {
        size_t sequence_length;
        if (str[i] <= 0x7F)
            sequence_length = 1;
        else if ((str[i] & 0xE0) == 0xC0)
            sequence_length = 2;
        else if ((str[i] & 0xF0) == 0xE0)
            sequence_length = 3;
        else if ((str[i] & 0xF8) == 0xF0)
            sequence_length = 4;
        else
        {
            memmove(str + i, str + i + 1, len - i - 1);
            len--;
            continue;
        }

        if (i + sequence_length > len || !is_valid_utf8(str + i, sequence_length))
        {
            memmove(str + i, str + i + sequence_length, len - i - sequence_length);
            len -= sequence_length;
            continue;
        }

        i += sequence_length;
    }

    str[len] = '\0';
}

int main()
{
    unsigned char str[] = "Hello\xFF World";
    printf("Original string: %s\n", str);
    remove_invalid_utf8(str, strlen((char *)str));
    printf("Modified string: %s\n", str);
    return 0;
}