/*
 *  fileselector - A simple file selector for the Jlime project.
 *      
 *  Copyright 2010-2013 Rafael Ignacio Zurita <rafaelignacio.zurita@gmail.com>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 *
 */

#define _SVID_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <sys/types.h>
#include <dirent.h>

#include <SDL.h>
#include <SDL/SDL_ttf.h>

#define ROW_HEIGHT 24

#define BLACK      0, 0, 0
#define BLUE       114, 159, 207
#define DARK_GRAY  60, 60, 60
#define LIGHT_GRAY 211, 215, 207

const SDL_VideoInfo *video_info;
SDL_Surface *scr;
TTF_Font *font;

char *fnames[1024], *filter_list[32], *cur_dir;
int hidden = 1, filter = 0, num_rows = 0, num_filters = 0;
int dir_index = 0, num_files = 0, scr_index = 1;

void usage(char *name)
{
	printf("Usage: %s -f [filter] -d [directory]\n", name);
	printf("   -f: Only show files matching the comma-seperated filter list.\n");
	printf("   -d: Start fileselector in selected directory.\n");
	exit(1);
}

void quit(int code)
{
	TTF_CloseFont(font);
	TTF_Quit();

	SDL_Quit();
	exit(code);
}

int filter_check(const char *file, const char *ext)
{
	size_t file_length = strlen(file), ext_length = strlen(ext);

	if (ext_length >= file_length)
		return 0;
	
	return (strcmp(&file[file_length - ext_length], ext)) == 0;
}

int directory_check(const struct dirent *d)
{
	DIR *directory;

	if (hidden)
		if (d->d_name[0] == '.' && d->d_name[1] != '.')
			return 0;

	if (strcmp(d->d_name, "."))
		if (directory = opendir(d->d_name)) {
			closedir(directory);
			return 1;
		} else {
			closedir(directory);
			return 0;
		}
	else
		return 0;
}

int file_check(const struct dirent *d)
{
	DIR *directory;
	int n;

	if (hidden)
		if (d->d_name[0] == '.')
			return 0;

	if (directory = opendir(d->d_name)) {
		closedir(directory);
		return 0;
	} else {
		closedir(directory);
		/* Filter files using array built with -f [filter] option */
		if (filter && num_filters) {
			for (n = 0; n < num_filters; n++) {
				if ((filter_check(d->d_name, filter_list[n])))
					return 1;
			}
			return 0;
		} else {
			return 1;
		}
	}
}

int fill_fname_array(char *data_dir)
{
	struct dirent **namelist;
	int n, d, f;

	/* Scan for folders first */
	n = scandir(data_dir, &namelist, directory_check, versionsort);
	if (n < 0)
		perror("scandir");
	else {
		d = n;
		while (n--) {
			fnames[n] = strdup(namelist[n]->d_name);
			free(namelist[n]);
		}

		free(namelist);
	}

	/* Scan for files, and append them to the folders list */
	n = scandir(data_dir, &namelist, file_check, versionsort);
	if (n < 0)
		perror("scandir");
	else {
		f = n;
		while (n--) {
			fnames[n + d] = strdup(namelist[n]->d_name);
			free(namelist[n]);
		}

		free(namelist);
	}

	return (d + f);
}

void draw_text(const char text[80], int x, int y, SDL_Color foregroundColor, SDL_Color bgcolor)
{
	SDL_Surface* textSurface = TTF_RenderUTF8_Shaded(font, text, foregroundColor, bgcolor); 
	SDL_Rect textLocation = { x, y, 0, 0 };

	SDL_BlitSurface(textSurface, NULL, scr, &textLocation);
	SDL_FreeSurface(textSurface);
}

void draw_bg(void)
{
	int i;
	int ngr = 1;
	int nHEIGHT = video_info->current_h / num_rows - 1;
	int nWIDTH = video_info->current_w;

	SDL_Color bgcolor = { BLACK };
	SDL_Rect item;
	SDL_Rect bg = { 0, 0, video_info->current_w, video_info->current_h };

	bg.x = 0;
	bg.y = 0;
	bg.w = video_info->current_w;
	bg.h = video_info->current_h;

	SDL_FillRect(scr, &bg, SDL_MapRGB(scr->format, DARK_GRAY));

	for (i = 0; i <= num_rows; i++) {
		item.x = 0;
		item.y = i * nHEIGHT + i;
		item.w = nWIDTH;
		item.h = nHEIGHT;

		SDL_FillRect(scr, &item, SDL_MapRGB(scr->format, BLACK));
	}
}

void show_names(void)
{
	int i, y, x = 20;
	int ngr = 1;
	int nHEIGHT = video_info->current_h / num_rows - 1;
	int nWIDTH = video_info->current_w;
	DIR *directory;

	SDL_Color bgcolor = { BLACK };	/* Text background color */
	SDL_Color bgcolor2 = { LIGHT_GRAY };	/* Selected text bg color */
	SDL_Color cnormal = { LIGHT_GRAY };	/* File text color */
	SDL_Color cdnormal = { BLUE };	/* Folder text Color */
	SDL_Color current = { BLACK };	/* Selected text color */
	SDL_Rect item;

	for (i = 0; i < num_rows; i++) {
		y = i * (video_info->current_h / num_rows) + ((nHEIGHT - 12) / 2);

		if (i == (scr_index - 1)) {
			item.x = 0;
			item.y = (ngr * i + nHEIGHT * (i));
			item.w = nWIDTH;
			item.h = nHEIGHT;
			SDL_FillRect(scr, &item, SDL_MapRGB(scr->format, LIGHT_GRAY));
		}

		if (dir_index - (scr_index - 1) + i < num_files) {
			directory = opendir(fnames[dir_index - (scr_index - 1) + i]);

			if (directory) {
				closedir(directory);

				if (i == (scr_index - 1)) {
					draw_text("[DIR] ", x, y, current, bgcolor2);
					draw_text(fnames[dir_index - (scr_index - 1) + i], x + 32, y, current, bgcolor2);
				} else {
					draw_text("[DIR] ", x, y, cdnormal, bgcolor);
					draw_text(fnames[dir_index - (scr_index - 1) + i], x + 32, y, cdnormal, bgcolor);
				}
			} else {
				if (i == (scr_index - 1))
					draw_text(fnames[dir_index - (scr_index - 1) + i], x, y, current, bgcolor2);
				else
					draw_text(fnames[dir_index - (scr_index - 1) + i], x, y, cnormal, bgcolor);
			}
		}
	}
}

void process_events(void) 
{
	SDL_Event event;
	DIR *directory;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			switch(event.key.keysym.sym) {
			case SDLK_f:
				if (num_filters) {
					if (filter)
						filter = 0;
					else
						filter = 1;
					dir_index = 0, scr_index = 1;
					num_files = fill_fname_array(".");
				}
				break;
			case SDLK_h:
				if (hidden)
					hidden = 0;
				else
					hidden = 1;
				dir_index = 0, scr_index = 1;
				num_files = fill_fname_array(".");
				break;
			case SDLK_BACKSPACE:
				chdir(fnames[0]);

				free(cur_dir);
				cur_dir = get_current_dir_name();
				SDL_WM_SetCaption(cur_dir, "fileselector");

				dir_index = 0, scr_index = 1;
				num_files = fill_fname_array(".");
				break;
			case SDLK_PAGEUP:
				dir_index = dir_index - 10;
				if (dir_index < 0)
					dir_index = 0;

				scr_index = scr_index - 10;
				if (scr_index < 1)
					scr_index = 1;
				break;
			case SDLK_PAGEDOWN:
				dir_index = dir_index + 10;
				if (dir_index > num_files - 1)
					dir_index = num_files - 1;
				else {
					scr_index = scr_index + 10;
					if (scr_index > num_rows)
						scr_index = num_rows;
				};
				break;
			case SDLK_UP:
				/* Shift + Up = Page Up */
				if (event.key.keysym.mod == KMOD_LSHIFT) {
					dir_index = dir_index - 10;
					if (dir_index < 0)
						dir_index = 0;

					scr_index = scr_index - 10;
					if (scr_index < 1)
						scr_index = 1;
				} else {
					dir_index = dir_index - 1;
					if (dir_index < 0)
						dir_index = 0;

					scr_index = scr_index - 1;
					if (scr_index < 1)
						scr_index = 1;
				}
				break;
			case SDLK_DOWN:
				/* Shift + Down = Page Down */
				if (event.key.keysym.mod == KMOD_LSHIFT) {
					dir_index = dir_index + 10;
					if (dir_index > num_files - 1)
						dir_index = num_files - 1;
					else {
						scr_index = scr_index + 10;
						if (scr_index > num_rows)
							scr_index = num_rows;
					};
				} else {
					dir_index = dir_index + 1;
					if (dir_index > num_files - 1)
						dir_index = dir_index - 1;
					else {
						scr_index = scr_index + 1;
						if (scr_index > num_rows)
							scr_index = num_rows;
					};
				}
				break;
			case SDLK_RETURN:
				directory = opendir(fnames[dir_index]);
				if (directory) {
					closedir(directory);
					chdir(fnames[dir_index]);

					free(cur_dir);
					cur_dir = get_current_dir_name();
					SDL_WM_SetCaption(cur_dir, "fileselector");

					dir_index = 0, scr_index = 1;
					num_files = fill_fname_array(".");
				} else {
					printf ("%s/%s", cur_dir, fnames[dir_index]);
					quit(0);
				};
				break;
			case SDLK_q:
			case SDLK_ESCAPE:
				quit(1);
				break;
			}
			draw_bg();
			show_names();
			break;
		case SDL_QUIT:
			quit(1);
			break;
		case SDL_VIDEORESIZE:
			scr = SDL_SetVideoMode(event.resize.w, event.resize.h,
					       video_info->vfmt->BitsPerPixel,
					       SDL_SWSURFACE | SDL_RESIZABLE);
			if (scr == NULL) {
				printf("Error: %s\n", SDL_GetError());
				exit(2);
			}
			num_rows = event.resize.h / ROW_HEIGHT;
			draw_bg();
			show_names();
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	struct dirent *dir;
	char *workdir = NULL;
	DIR *dp;
	int i;

	/* Parse command-line options */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch ((int) argv[i][1]) {
			case 'd':
				if (argv[i + 1] == NULL || argv[i + 1][0] == '-') {
					printf("Error: No directory specified!\n");
					exit(1);
				} else if (dp = opendir(argv[++i])) {
					closedir(dp);
					workdir = argv[i];
				} else {
					closedir(dp);
					printf("Error: Specified directory path is invalid!\n");
					exit(1);
				}
				break;
			case 'f':
				if (argv[i + 1] == NULL || argv[i + 1][0] == '-') {
					printf("Error: No extensions specified!\n");
					exit(1);
				} else {
					int n;
					char *token, tmp[64], ext[64];
					for (;;) {
						token = strtok(argv[++i], ",");
						for (n = 0; token != NULL; n++) {
							sscanf(token, "%s", tmp);
							sprintf(ext, ".%s", tmp);
							filter_list[n] = strdup(ext);
							token = strtok(NULL, ",");
						}
						filter = 1;
						num_filters = n;
						break;
					}
				}
				break;
			default:
				usage(argv[0]);
			}
		} else {
			usage(argv[0]);
		}
	}

	if (workdir == NULL)
		workdir = get_current_dir_name();

	if (SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		printf("Error: %s\n", SDL_GetError());
		exit(2);
	}

	video_info = SDL_GetVideoInfo();
	num_rows = video_info->current_h / ROW_HEIGHT;

	scr = SDL_SetVideoMode(video_info->current_w, video_info->current_h,
			       video_info->vfmt->BitsPerPixel,
			       SDL_SWSURFACE | SDL_RESIZABLE);
	if (scr == NULL) {
		printf("Error: %s\n", SDL_GetError());
		exit(2);
	}

	TTF_Init();

	font = TTF_OpenFont("/usr/share/fonts/truetype/DejaVuSansCondensed-Bold.ttf", 10);
	
	if (font == NULL) {
		font = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSansCondensed-Bold.ttf", 10);
		if (font == NULL) {
			font = TTF_OpenFont("DejaVuSansCondensed-Bold.ttf", 10);
				if (font == NULL) {
					printf("Error: No fonts found!\nExiting...\n");
					quit(2);
				}
		}
	}

	SDL_EnableKeyRepeat(300, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_ShowCursor(SDL_DISABLE);

	SDL_WM_SetCaption(workdir, "fileselector");

	chdir(workdir);

	num_files = fill_fname_array(workdir);
	cur_dir = get_current_dir_name();

	draw_bg();
	show_names();

	for (;;) {
		process_events();

		SDL_Delay(60);

		SDL_Flip(scr);
	}

	quit(0);
}
