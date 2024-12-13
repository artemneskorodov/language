#ifndef COLORS_H
#define COLORS_H

enum color_t {
    RED_TEXT    ,
    GREEN_TEXT  ,
    YELLOW_TEXT ,
    BLUE_TEXT   ,
    MAGENTA_TEXT,
    CYAN_TEXT   ,
    WHITE_TEXT  ,
    DEFAULT_TEXT,
};

enum background_t {
    BLACK_BACKGROUND  ,
    RED_BACKGROUND    ,
    GREEN_BACKGROUND  ,
    YELLOW_BACKGROUND ,
    BLUE_BACKGROUND   ,
    PURPLE_BACKGROUND ,
    CYAN_BACKGROUND   ,
    WHITE_BACKGROUND  ,
    DEFAULT_BACKGROUND,
};

enum boldness_t {
    BOLD_TEXT  ,
    NORMAL_TEXT,
};

int  color_printf  (color_t      color,
                    boldness_t   is_bold,
                    background_t background,
                    const char * format, ...);

int  color_vprintf (color_t      color,
                    boldness_t   is_bold,
                    background_t background,
                    const char  *format,
                    va_list      args);

int  print_error   (const char *message, ...);

#endif
