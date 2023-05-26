#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE   	(!FALSE)
#endif

#include <libgimp/gimp.h>
#include <string.h>
#include <stdio.h>
#include <regex.h>
#include <glob.h>
#include <pwd.h>
#include "trim.c"
#include <iconv.h>

struct Text {
	gchar value[256];
};

struct Texts {
	gint length;
	struct Text text[30];
};

struct Setting {
	gchar style_file[256];
	gchar default_fontname[100];
	gdouble font_size;
	gchar target[256];
	gint position;
};

struct Font {
	gchar fontname[100];
	gchar tag[2];
	gint black;
};

struct Fonts {
	gint length;
	struct Font fonts[50];
};

static void run (const gchar *name, gint nparams, const GimpParam *param, gint *nreturn_vals, GimpParam **return_vals);
static void query (void);
char **get_filenames(const gchar *input_dir);
void get_text(gint page_number, gchar *text_file, struct Texts *texts);
gint is_page(gchar *text, gint page_number);
gint is_space(gchar *text);
gint get_biggest_word(const gchar *text);
void get_setting();
void get_style();
void slice(const gchar * str, gchar * buffer, size_t start, size_t end);
gint only_tag(const gchar *tag);

GimpPlugInInfo PLUG_IN_INFO = {
	NULL,
	NULL,
	query,
	run
};

struct Setting setting = {
	"",
	"",
	28,
	"",
	0,
};

struct Fonts fonts;
char homedir[256];

static void query (void)
{
	static GimpParamDef args[] = {
		{
			GIMP_PDB_STRING,
			"input-dir",
			"Input Dir"
		},
		{
			GIMP_PDB_STRING,
			"output-dir",
			"Output Dir"
		},
		{
			GIMP_PDB_STRING,
			"text-file",
			"Text File"
		}
	};

	gimp_install_procedure (
		"plug-in-save-image",
		"Add text and save image!",
		"Add text in an image and save image as xcf",
		"Alexandre dos Santos Alves",
		"Copyright Alexandre",
		"2023",
		"_Save Image",
		"RGB*, GRAY*",
		GIMP_PLUGIN,
		G_N_ELEMENTS (args), 0,
		args, NULL);
}

static void run (
	const gchar      *name,
	gint              nparams,
	const GimpParam  *param,
	gint             *nreturn_vals,
	GimpParam       **return_vals)
{

	static GimpParam  values[1];
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	GimpRunMode       run_mode;
	GimpDrawable     *drawable;

	*nreturn_vals = 1;
	*return_vals  = values;

	values[0].type = GIMP_PDB_STATUS;
	values[0].data.d_status = status;

	char *homedir_tmp;

	if ((homedir_tmp = getenv("HOME")) == NULL) {
		homedir_tmp = getpwuid(getuid())->pw_dir;
	}

	strcpy(homedir, homedir_tmp);
	strcat(homedir, "/.gimp_setting_type_tool");

	gimp_debug_timer_start();
	g_printerr("Started\n");

	gchar *input_dir = param[0].data.d_string;
	gchar *output_dir = param[1].data.d_string;
	gchar *text_file = param[2].data.d_string;

	gchar **filenames = get_filenames(input_dir);

	while(*filenames) {
		GError *error = NULL;
		gchar *filename = strrchr(*filenames, '/');
		gint page_number;
		gint image, width_image;
		gchar delim[] = " ";
		gint text_layer;
		gdouble position[2] = {0,0};

		struct Texts texts = {0};

		get_setting();
		get_style();

		if (error) {
			g_printerr("Error: %s\n", error->message);
			g_error_free(error);
			gimp_quit();
		}

		image = gimp_file_load(GIMP_RUN_NONINTERACTIVE, *filenames, *filenames);
		width_image = gimp_image_width(image);

		if (filename) {
	        filename++; // Move the pointer to the character after the '/'
	        if (sscanf(filename, "%d", &page_number) != 1) {
	        	g_printerr("Error: No number found in filename\n");
	        	gimp_quit();
	        }
	      } else {
	      	g_printerr("Error: No '/' found in path\n");
	      	gimp_quit();
	      }

	      get_text(page_number, text_file, &texts);

	      for (int i = 0; i < texts.length; ++i) {
	      	gchar *text_trim, *text_splited;
	      	gchar current_text[256];
	      	strcpy(current_text, "");

	      	gint width = get_biggest_word(texts.text[i].value);

	      	struct Font current_font = {
	      		"",
	      		"",
	      		1,
	      	};

	      	strcpy(current_font.fontname, setting.default_fontname);

	      	text_trim = trim(texts.text[i].value);
	      	text_splited = strtok(text_trim, delim);

	      	while (text_splited != NULL) {
	      		int is_tag = FALSE;
	      		if (only_tag(text_splited)) {
	      			for (int i = 0; i < fonts.length; ++i) {
	      				if (strcmp(fonts.fonts[i].tag, text_splited) == 0) {
	      					if (strcmp(fonts.fonts[i].fontname, "Black") == 0) {
	      						current_font.black = 1;
	      					} else if (strcmp(fonts.fonts[i].fontname, "White") == 0) {
	      						current_font.black = 0;
	      					} else {
	      						strcpy(current_font.fontname, fonts.fonts[i].fontname);
	      					}
	      					is_tag = TRUE;
	      					break;
	      				}
	      			}
	      			if (is_tag) {
	      				text_splited = strtok(NULL, delim);
	      				continue;
	      			}
	      		}
	      		strcat(current_text, text_splited);

	      		text_splited = strtok(NULL, delim);
	      		if (text_splited != NULL) {
	      			strcat(current_text, " ");
	      		}
	      	}

	      	text_layer = gimp_text_fontname(image, -1, position[0], position[1], current_text, 0, TRUE, setting.font_size, GIMP_PIXELS, current_font.fontname);

	      	position[0] += width;

	      	if (position[0] > width_image) {
	      		position[0] = 0;
	      		position[1] += 310;
	      	}

	      	gimp_text_layer_set_justification(text_layer, GIMP_TEXT_JUSTIFY_CENTER);
	      	gimp_text_layer_resize(text_layer, width + 25, 300);
	      	if (current_font.black) {
	      		GimpRGB color = { 0, 0, 0, 1 };
	      		gimp_text_layer_set_color(text_layer, &color);
	      	} else {
	      		GimpRGB color = { 255, 255, 255, 1 };
	      		gimp_text_layer_set_color(text_layer, &color);
	      	}
	      }

	      gint32 drawable = gimp_image_get_active_drawable(image);

	      gchar *xcf_filename = g_strdup_printf("%s/%d.xcf", output_dir, page_number);

	      gimp_file_save(GIMP_RUN_NONINTERACTIVE, image, drawable, xcf_filename, xcf_filename);
	      gimp_image_delete(image);

	      filenames++;
	    }

	    gimp_debug_timer_end();
	    g_printerr("Finished\n");
	  }

	  char **get_filenames(const char *input_dir) {
	  	char **files;
	  	glob_t gstruct;
	  	int r;

	  	r = glob(input_dir, GLOB_ERR , NULL, &gstruct);
    /* check for errors */
	  	if( r!=0 ) {
	  		char error[100];
	  		if( r==GLOB_NOMATCH )
	  			snprintf(error, sizeof error, "No matches - %s (%d)", __FILE__ ,__LINE__);
	  		else
	  			snprintf(error, sizeof error, "Some kinda glob error - %s (%d)", __FILE__ ,__LINE__);

	  		gimp_quit();
	  	}

	  	files = gstruct.gl_pathv;

	  	return files;
	  }

	  void get_text(gint page_number, gchar *text_file, struct Texts *texts) {
	  	FILE* file_ptr;
	  	static gchar str[256];
	gint status = FALSE; // status for get text
	file_ptr = fopen(text_file, "r");

	if (NULL == file_ptr) {
		char error[100];
		snprintf(error, sizeof error, "unable to access file - %s (%d)", __FILE__ ,__LINE__);
		gimp_message(error);
		gimp_quit();
	}

	while (fgets(str, 256, file_ptr) != NULL) {
		if (is_space(str)) {
			continue;
		}
		if (is_page(str, page_number)) {
			status = TRUE;
			continue;
		} else if (is_page(str, page_number + 1)) {
			status = FALSE;
			break;
		}

		if (status) {
			strcpy(texts->text[texts->length].value, str);
			texts->length++;
		}
	}

	fclose(file_ptr);
}

gint is_page(gchar *text, gint page_number) {
	regex_t reegex;
	gint value;

	value = regcomp( &reegex, "^##", 0 );
	value = regexec( &reegex, text, 0, NULL, 0 );

	if (value == 0) {
		char *ptr = text;
		while (*ptr && (*ptr < '0' || *ptr > '9')) {
			ptr++;
		}
		if (atoi(ptr) == page_number) {
			return TRUE;
		}
	}
	return FALSE;
}

gint is_space(gchar *text) {
	regex_t reegex;
	gint value;
	value = regcomp( &reegex, "^[ \t\n\r\f\v]*$", 0 );

	value = regexec( &reegex, text,0, NULL, 0 );

	if (value == 0) {
		return TRUE;
	}
	return FALSE;
}

gint get_biggest_word(const gchar *text) {
	const char *biggest_word = NULL;
	size_t biggest_word_len = 0;

	const char *word_start = text;
	for (const char *p = text; ; p++) {
		if (!*p || isspace(*p)) {
			size_t word_len = p - word_start;
			if (word_len > biggest_word_len) {
				biggest_word = word_start;
				biggest_word_len = word_len;
			}
			if (!*p) break;
			word_start = p + 1;
		}
	}

	char *result = malloc(biggest_word_len + 1);
	memcpy(result, biggest_word, biggest_word_len);
	result[biggest_word_len] = '\0';

	gint width, height, ascent, descent;
	gimp_text_get_extents_fontname(result, setting.font_size, GIMP_PIXELS, setting.default_fontname, &width, &height, &ascent, &descent);

	free(result);

	return width;
}

void get_setting() {
	FILE* file_ptr;
	gchar str[256], *setting_str;
	gchar delim[] = "|";
	gint count = 0;

	file_ptr = fopen(homedir, "r");

	if (NULL == file_ptr) {
		char error[100];
		snprintf(error, sizeof error, "unable to access file - %s (%d)", __FILE__ ,__LINE__);
		gimp_message(error);
		gimp_quit();
	}

	fgets(str, 256, file_ptr);

	setting_str = strtok(str, delim);

	while (setting_str != NULL) {

		switch (count) {
		case 0:
			strcpy(setting.default_fontname, setting_str);
			break;
		case 1:
			setting.font_size = atoi(setting_str);
			break;
		case 2:
			strcpy(setting.target, setting_str);
			break;
		case 3:
			setting.position = atoi(setting_str);
			break;
		case 4:
			strcpy(setting.style_file, setting_str);
			break;
		}

		count++;
		setting_str = strtok(NULL, delim);
	}

	fclose(file_ptr);
}

void get_style() {
	FILE* file_ptr;
	gchar str[100];
	file_ptr = fopen(setting.style_file, "r");

	if (NULL == file_ptr) {
		gchar error[100];
		snprintf(error, sizeof error, "unable to access file - %s (%d)", __FILE__ ,__LINE__);
		gimp_message(error);
		gimp_quit();
	}

	fonts.length = 0;

	while (fgets(str, 100, file_ptr) != NULL) {
		char *text_trim = trim(str);
		slice(text_trim, fonts.fonts[fonts.length].tag, 0, 1);
		slice(text_trim, fonts.fonts[fonts.length].fontname, 3, strlen(text_trim));
		fonts.length++;
	}

	fclose(file_ptr);
}

void slice(const gchar * str, gchar * buffer, size_t start, size_t end) {
	size_t j = 0;
	for ( size_t i = start; i <= end; ++i ) {
		buffer[j++] = str[i];
	}
	buffer[j] = 0;
}

gint only_tag(const gchar *tag) {
	regex_t reegex;
	gint value;
	value = regcomp( &reegex, "[^A-Za-z0-9 ]", 0 );

	value = regexec( &reegex, tag, 0, NULL, 0 );

	if (value == 0) {
		return TRUE;
	}
	return FALSE;
}

MAIN()