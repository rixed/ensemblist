/*
Copyright (C) 2003 Cedric Cellier, Dominique Lavault

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "modes.h"
#include "main.h"
#include "data.h"
#include "memspool.h"
#include "log.h"
#include "system.h"
#include "draw.h"
#include "opengl.h"
#include "sound.h"
#include "user.h"

#define NAME "Ensemblist"
#define VERSION "20031203"
#ifdef NDEBUG
#define DEBUGSTR ""
#else
#define DEBUGSTR " - DEBUG version"
#endif

#ifdef _WIN32
#define DATADIR .\\datas
#endif

int glut_fenLong = 800;
int glut_fenHaut = 600;
int glut_fenPosX = -1;
int glut_fenPosY = -1;
unsigned glut_interval = 25;	/* msec */
unsigned long glut_mtime = 0;
static unsigned glut_fenMode = /*GLUT_DOUBLE |*/ GLUT_RGBA | GLUT_DEPTH;// | GLUT_STENCIL;
static const char *glut_fenName = "Ensemblist";
static int glut_top_window;

static void glut_reshape(int w, int h)
{
	glut_fenLong=w;
	glut_fenHaut=h;
	glViewport(0,0,w,h);
}

static void glut_display(void)
{
	if (NULL!=mode_display[mode]) {
		mode_display[mode]();
		glutSwapBuffers();
	}
}

float glut_mouse_x = 0;
float glut_mouse_y = 0;
char glut_mouse_changed = 0;
int glut_mouse_button = 0;

void convert_pixel_to_percent(int x, int y, float *xx, float *yy) {
	x *= 2;
	y *= 2;
	x -= glut_fenLong;
	y -= glut_fenHaut;
	*xx = (float)x/glut_fenLong;
	*yy = (float)y/glut_fenHaut;
}

static void update_glut_mouse(int x, int y) {
	float xx, yy;
	convert_pixel_to_percent(x, y, &xx, &yy);
	glut_mouse_x = xx;
	glut_mouse_y = yy;
}
	

static void glut_mouse_motion(int x, int y) {
	if (NULL!=mode_mouse_motion[mode]) mode_mouse_motion[mode](x, y);
	update_glut_mouse(x,y);
	glut_mouse_changed = 1;
}

static void glut_mouse(int button, int state, int x, int y) {
	update_glut_mouse(x,y);
	switch (state) {
		case GLUT_UP:
			switch (button) {
				case GLUT_LEFT_BUTTON:
					glut_mouse_button &= ~1;
					if (NULL!=mode_mouse[mode]) mode_mouse[mode](1,0);
					break;
				case GLUT_MIDDLE_BUTTON:
					glut_mouse_button &= ~2;
					if (NULL!=mode_mouse[mode]) mode_mouse[mode](2,0);
					break;
				case GLUT_RIGHT_BUTTON:
					glut_mouse_button &= ~4;
					if (NULL!=mode_mouse[mode]) mode_mouse[mode](4,0);
					break;
			}
			break;
		case GLUT_DOWN:
			switch (button) {
				case GLUT_LEFT_BUTTON:
					glut_mouse_button |= 1;
					if (NULL!=mode_mouse[mode]) mode_mouse[mode](1,1);
					break;
				case GLUT_MIDDLE_BUTTON:
					glut_mouse_button |= 2;
					if (NULL!=mode_mouse[mode]) mode_mouse[mode](2,1);
					break;
				case GLUT_RIGHT_BUTTON:
					glut_mouse_button |= 4;
					if (NULL!=mode_mouse[mode]) mode_mouse[mode](4,1);
					break;
			}
			break;
	}
}

static void glut_key(unsigned char c, int x, int y) {
	if (NULL!=mode_key[mode]) {
		float xx, yy;
		convert_pixel_to_percent(x, y, &xx, &yy);
		mode_key[mode]((int)c, xx, yy, 0);
	}
}

static void glut_special(int c, int x, int y) {
	if (NULL!=mode_key[mode]) {
		float xx, yy;
		convert_pixel_to_percent(x, y, &xx, &yy);
		mode_key[mode](c, xx, yy, 1);
	}
}
	
static void glut_timer(int timer) {
	glut_mtime += glut_interval;
	glutPostRedisplay();	/* will call glut_display */
	sound_update();
	glutTimerFunc(glut_interval, glut_timer, timer);	/* register myself again */
}

static void glut_go(int nb_args, char **args) {
	glutInit(&nb_args, args);
	glutInitWindowSize(glut_fenLong, glut_fenHaut);
	glutInitWindowPosition(glut_fenPosX, glut_fenPosY);
	glutInitDisplayMode(glut_fenMode);
	glut_top_window = glutCreateWindow(glut_fenName);
	glClearColor(0.78, 0.75, 0.80, 0.0);
	glutReshapeFunc(glut_reshape);
	glutDisplayFunc(glut_display);
	glutMouseFunc(glut_mouse);
	glutMotionFunc(glut_mouse_motion);
	glutPassiveMotionFunc(glut_mouse_motion);
	glutKeyboardFunc(glut_key);
	glutSpecialFunc(glut_special);
	glutTimerFunc(glut_interval, glut_timer, 1);
	glutMainLoop();
}


int main(int nb_args, char **args) {
	int a, with_sounds = 1, debug_level = GLTV_LOG_IMPORTANT;
	puts(NAME " version " VERSION DEBUGSTR ",\n" \
"Copyright (C) 2003 Cedric Cellier, Dominique Lavault\n" \
"Ensemblist comes with ABSOLUTELY NO WARRANTY;\n" \
"This is free software, and you are welcome to redistribute it under certain conditions;\n" \
"See http://www.gnu.org/licenses/gpl.txt for details.\n");
	/* command line */
	for (a=1; a<nb_args; a++) {
		if (0==strncasecmp(args[a], "--silent", 8)) {
			with_sounds = 0;
		} else if (0==strncasecmp(args[a], "--debug", 7)) {
			debug_level = GLTV_LOG_DEBUG;
		} else if (0==strncasecmp(args[a], "--ktx", 5)) {
			with_ktx = 1;
		} else if (0==strncasecmp(args[a], "--wgl", 5)) {
			with_wgl = 1;
		} else if (0==strncasecmp(args[a], "--help", 6)) {
			printf("\n%s [--silent] [--debug] [--ktx | --wgl] [--help]\n\n" \
					"  --silent : no sound\n" \
					"  --debug : verbose\n" \
					"  --ktx --wgl : use one of these extentions (do not)\n" \
					"  --help : this\n\n", args[0]);
			exit(0);
		} else {
			puts("Unknown command line (--help for help)");
			exit(1);
		}
	}
	/* inits */
	gltv_log_init(debug_level);
	gltv_memspool_init(150000);
	atexit(gltv_memspool_end);
	if (!sys_goto_dir(XSTR(DATADIR))) {
error_chdir1:
		gltv_log_fatal("Cannot chdir to " XSTR(DATADIR));
	}
	/* read user's file */
	user_read();
	/* read data files (primitives, enigms) */
	data_read_all();
	/* then userland additionnal enigms */
	if (!sys_goto_dir(user_rc_dir)) {
		gltv_log_fatal("Cannot chdir to %s", user_rc_dir);
	}
	data_read_userland();
	if (!sys_goto_dir(XSTR(DATADIR))) goto error_chdir1;	// go back to datadir so that the modes can load things.
	/* sound init */
	sound_init(with_sounds);
	atexit(sound_end);
	/* run Forest, run */
	glut_go(nb_args, args);
	
	return 0;	/* will never return here... */
}

#ifdef _WINDOWS
char g_sAppName[256];
char g_sBuf[4096];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR szCmdLine, int iCmdShow)
{
	char argv0[256];
	char *argv_all;
	char *argv[256];
	int ret_val;
	int argc = 0;

	BOOL fInArg = FALSE;
	BOOL fInQuote;
	int i,j;

	GetModuleFileName(hInstance, argv0, sizeof (argv0));
	strcpy( g_sAppName, argv0 );
	argv[argc++] = argv0;

	argv_all = (char *) malloc (strlen(szCmdLine)+1);

	j = 0;
	for (i=0; (char) szCmdLine[i]; i++)
	{
		char ch = (char) szCmdLine[i];
		if (fInArg)
		{
			if (fInQuote && ch=='\"' || !fInQuote && (ch==' ' || ch=='\t'))
			{
				fInArg = FALSE;
				argv_all[j++] = '\0';
			}
			else
			{
				argv_all[j++] = ch;
			}
		}
		else
		{
			if (ch!=' ' && ch!='\t')
			{
				fInArg = TRUE;
				fInQuote = ch=='\"';
				if (!fInQuote)
					i--;
				argv[argc] = argv_all + j;
				argc++;
			}
		}
	}

	if (fInArg)
		argv_all[j++] = '\0';

	argv[argc] = NULL;

	ret_val = main(argc, argv);

	free (argv_all);

	return ret_val;
}
#endif
