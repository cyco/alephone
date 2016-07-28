/*

	Copyright (C) 1991-2001 and beyond by Bungie Studios, Inc.
	and the "Aleph One" developers.
 
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This license is contained in the file "COPYING",
	which is included with this source code; it is available online at
	http://www.gnu.org/licenses/gpl.html

*/

/*
 *  shell.cpp - Main game loop and input handling
 */

#include "cseries.h"

#include "map.h"
#include "monsters.h"
#include "player.h"
#include "render.h"
#include "shell.h"
#include "interface.h"
#include "SoundManager.h"
#include "fades.h"
#include "screen.h"
#include "Music.h"
#include "images.h"
#include "vbl.h"
#include "preferences.h"
#include "tags.h" /* for scenario file type.. */
#include "network_sound.h"
#include "mouse.h"
#include "joystick.h"
#include "screen_drawing.h"
#include "computer_interface.h"
#include "game_wad.h" /* yuck... */
#include "game_window.h" /* for draw_interface() */
#include "extensions.h"
#include "items.h"
#include "interface_menus.h"
#include "weapons.h"
#include "lua_script.h"

#include "Crosshairs.h"
#include "OGL_Render.h"
#include "OGL_Blitter.h"
#include "XML_ParseTreeRoot.h"
#include "FileHandler.h"
#include "Plugins.h"
#include "FilmProfile.h"

#include "mytm.h"	// mytm_initialize(), for platform-specific shell_*.h

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <vector>

#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "resource_manager.h"
#include "sdl_dialogs.h"
#include "sdl_fonts.h"
#include "sdl_widgets.h"

#include "DefaultStringSets.h"
#include "TextStrings.h"

#ifdef HAVE_CONFIG_H
#include "confpaths.h"
#endif

#include <ctime>
#include <exception>
#include <algorithm>
#include <vector>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_OPENGL
#include "OGL_Headers.h"
#endif

#if !defined(DISABLE_NETWORKING)
#include <SDL_net.h>
#endif

#ifdef HAVE_PNG
#include "IMG_savepng.h"
#endif

#ifdef HAVE_SDL_IMAGE
#include <SDL_image.h>
#if defined(__WIN32__)
#include "alephone32.xpm"
#elif !(defined(__APPLE__) && defined(__MACH__))
#include "alephone.xpm"
#endif
#endif

#ifdef __WIN32__
#include <windows.h>
#include <shlobj.h>
#endif

#include "alephversion.h"

#include "Logging.h"
#include "network.h"
#include "Console.h"
#include "Movie.h"
#include "HTTP.h"
#include "WadImageCache.h"

// LP addition: whether or not the cheats are active
// Defined in shell_misc.cpp
extern bool CheatsActive;

// Application names
#if defined(__MACH__) && defined(__APPLE__)


#else
char application_name[] = A1_DISPLAY_NAME;
char application_identifier[] = "org.bungie.source.AlephOne";
#endif

#if defined(HAVE_BUNDLE_NAME)
// legacy bundle path
static const char sBundlePlaceholder[] = "AlephOneSDL.app/Contents/Resources/DataFiles";
#endif

// Command-line options
bool option_nogl = false;             // Disable OpenGL
bool option_nosound = false;          // Disable sound output
bool option_nogamma = false;	      // Disable gamma table effects (menu fades)
bool option_debug = false;
bool option_nojoystick = false;
bool insecure_lua = false;
static bool force_fullscreen = false; // Force fullscreen mode
static bool force_windowed = false;   // Force windowed mode

// Prototypes
static void main_event_loop(void);
extern int process_keyword_key(char key);
extern void handle_keyword(int type_of_cheat);

void PlayInterfaceButtonSound(short SoundID);

// From preprocess_map_sdl.cpp
extern bool get_default_music_spec(FileSpecifier &file);
extern bool get_default_theme_spec(FileSpecifier& file);

// From vbl_sdl.cpp
void execute_timer_tasks(uint32 time);

// Prototypes
static void initialize_marathon_music_handler(void);
static void process_event(const SDL_Event &event);

// cross-platform static variables
short vidmasterStringSetID = -1; // can be set with MML

static void usage(const char *prg_name)
{
	char msg[] =
#ifdef __WIN32__
	  "Command line switches:\n\n"
#else
	  "\nUsage: %s [options] [directory] [file]\n"
#endif
	  "\t[-h | --help]          Display this help message\n"
	  "\t[-v | --version]       Display the game version\n"
	  "\t[-d | --debug]         Allow saving of core files\n"
	  "\t                       (by disabling SDL parachute)\n"
	  "\t[-f | --fullscreen]    Run the game fullscreen\n"
	  "\t[-w | --windowed]      Run the game in a window\n"
#ifdef HAVE_OPENGL
	  "\t[-g | --nogl]          Do not use OpenGL\n"
#endif
	  "\t[-s | --nosound]       Do not access the sound card\n"
	  "\t[-m | --nogamma]       Disable gamma table effects (menu fades)\n"
          "\t[-j | --nojoystick]    Do not initialize joysticks\n"
	  // Documenting this might be a bad idea?
	  // "\t[-i | --insecure_lua]  Allow Lua netscripts to take over your computer\n"
	  "\tdirectory              Directory containing scenario data files\n"
          "\tfile                   Saved game to load or film to play\n"
	  "\nYou can also use the ALEPHONE_DATA environment variable to specify\n"
	  "the data directory.\n";

#ifdef __WIN32__
	MessageBox(NULL, msg, "Usage", MB_OK | MB_ICONINFORMATION);
#else
	printf(msg, prg_name);
#endif
	exit(0);
}

extern bool handle_open_replay(FileSpecifier& File);
extern bool load_and_start_game(FileSpecifier& file);

bool handle_open_document(const std::string& filename)
{
	bool done = false;
	FileSpecifier file(filename);
	switch (file.GetType())
	{
	case _typecode_scenario:
		set_map_file(file);
		break;
	case _typecode_savegame:
		if (load_and_start_game(file))
		{
			done = true;
		}
		break;
	case _typecode_film:
		if (handle_open_replay(file))
		{
			done = true;
		}
		break;
	case _typecode_physics:
		set_physics_file(file);
		break;
	case _typecode_shapes:
		open_shapes_file(file);
		break;
	case _typecode_sounds:
		SoundManager::instance()->OpenSoundFile(file);
		break;
	default:
		break;
	}
	
	return done;
}


#if defined(__APPLE__) && defined(__MACH__)
extern "C" {
	int shell_main(int argc, char **argv);
}
int shell_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif

               
static int char_is_not_filesafe(int c)
{
    return (c != ' ' && !std::isalnum(c));
}

bool networking_available(void)
{
#if !defined(DISABLE_NETWORKING)
	return true;
#else
	return false;
#endif
}

static void initialize_marathon_music_handler(void)
{
	FileSpecifier file;
	if (get_default_music_spec(file))
		Music::instance()->SetupIntroMusic(file);
}

bool quit_without_saving(void)
{
	dialog d;
	vertical_placer *placer = new vertical_placer;
	placer->dual_add (new w_static_text("Are you sure you wish to"), d);
	placer->dual_add (new w_static_text("cancel the game in progress?"), d);
	placer->add (new w_spacer(), true);
	
	horizontal_placer *button_placer = new horizontal_placer;
	w_button *default_button = new w_button("YES", dialog_ok, &d);
	button_placer->dual_add (default_button, d);
	button_placer->dual_add (new w_button("NO", dialog_cancel, &d), d);
	d.activate_widget(default_button);
	placer->add(button_placer, true);
	d.set_widget_placer(placer);
	return d.run() == 0;
}

// ZZZ: moved level-numbers widget into sdl_widgets for a wider audience.

const int32 AllPlayableLevels = _single_player_entry_point | _multiplayer_carnage_entry_point | _multiplayer_cooperative_entry_point | _kill_the_man_with_the_ball_entry_point | _king_of_hill_entry_point | _rugby_entry_point | _capture_the_flag_entry_point;

short get_level_number_from_user(void)
{
	// Get levels
	vector<entry_point> levels;
	if (!get_entry_points(levels, AllPlayableLevels)) {
		entry_point dummy;
		dummy.level_number = 0;
		strcpy(dummy.level_name, "Untitled Level");
		levels.push_back(dummy);
	}

	// Create dialog
	dialog d;
	vertical_placer *placer = new vertical_placer;
	if (vidmasterStringSetID != -1 && TS_IsPresent(vidmasterStringSetID) && TS_CountStrings(vidmasterStringSetID) > 0) {
		// if we there's a stringset present for it, load the message from there
		int num_lines = TS_CountStrings(vidmasterStringSetID);

		for (size_t i = 0; i < num_lines; i++) {
			bool message_font_title_color = true;
			const char *string = TS_GetCString(vidmasterStringSetID, i);
			if (!strncmp(string, "[QUOTE]", 7)) {
				string = string + 7;
				message_font_title_color = false;
			}
			if (!strlen(string))
				placer->add(new w_spacer(), true);
			else if (message_font_title_color)
				placer->dual_add(new w_static_text(string), d);
			else
				placer->dual_add(new w_static_text(string), d);
		}

	} else {
		// no stringset or no strings in stringset - use default message
		placer->dual_add(new w_static_text("Before proceeding any further, you"), d);
		placer->dual_add(new w_static_text ("must take the oath of the vidmaster:"), d);
		placer->add(new w_spacer(), true);
		placer->dual_add(new w_static_text("\xd2I pledge to punch all switches,"), d);
		placer->dual_add(new w_static_text("to never shoot where I could use grenades,"), d);
		placer->dual_add(new w_static_text("to admit the existence of no level"), d);
		placer->dual_add(new w_static_text("except Total Carnage,"), d);
		placer->dual_add(new w_static_text("to never use Caps Lock as my \xd4run\xd5 key,"), d);
		placer->dual_add(new w_static_text("and to never, ever, leave a single Bob alive.\xd3"), d);
	}

	placer->add(new w_spacer(), true);
	placer->dual_add(new w_static_text("Start at level:"), d);

	w_levels *level_w = new w_levels(levels, &d);
	placer->dual_add(level_w, d);
	placer->add(new w_spacer(), true);
	placer->dual_add(new w_button("CANCEL", dialog_cancel, &d), d);

	d.activate_widget(level_w);
	d.set_widget_placer(placer);

	// Run dialog
	short level;
	if (d.run() == 0)		// OK
		// Should do noncontiguous map files OK
		level = levels[level_w->get_selection()].level_number;
	else
		level = NONE;

	// Redraw main menu
	update_game_window();
	return level;
}

const uint32 TICKS_BETWEEN_EVENT_POLL = 16; // 60 Hz
static void main_event_loop(void)
{
	uint32 last_event_poll = 0;
	short game_state;

	while ((game_state = get_game_state()) != _quit_game) {
		uint32 cur_time = SDL_GetTicks();
		bool yield_time = false;
		bool poll_event = false;

		switch (game_state) {
			case _game_in_progress:
			case _change_level:
			  if (Console::instance()->input_active() || cur_time - last_event_poll >= TICKS_BETWEEN_EVENT_POLL) {
					poll_event = true;
					last_event_poll = cur_time;
			  } else {				  
					SDL_PumpEvents ();	// This ensures a responsive keyboard control
			  }
				break;

			case _display_intro_screens:
			case _display_main_menu:
			case _display_chapter_heading:
			case _display_prologue:
			case _display_epilogue:
			case _begin_display_of_epilogue:
			case _display_credits:
			case _display_intro_screens_for_demo:
			case _display_quit_screens:
			case _displaying_network_game_dialogs:
				yield_time = interface_fade_finished();
				poll_event = true;
				break;

			case _close_game:
			case _switch_demo:
			case _revert_game:
				yield_time = poll_event = true;
				break;
		}

		if (poll_event) {
			global_idle_proc();

			while (true) {
				SDL_Event event;
				bool found_event = SDL_PollEvent(&event);

				if (yield_time) {
					// The game is not in a "hot" state, yield time to other
					// processes by calling SDL_Delay() but only try for a maximum
					// of 30ms
					int num_tries = 0;
					while (!found_event && num_tries < 3) {
						SDL_Delay(10);
						found_event = SDL_PollEvent(&event);
						num_tries++;
					}
					yield_time = false;
				} else if (!found_event)
					break;

				if (found_event)
					process_event(event); 
			}
		}

		execute_timer_tasks(SDL_GetTicks());
		idle_game_state(SDL_GetTicks());

		if (game_state == _game_in_progress && !graphics_preferences->hog_the_cpu && (TICKS_PER_SECOND - (SDL_GetTicks() - cur_time)) > 10)
		{
			SDL_Delay(1);
		}
	}
}

static bool has_cheat_modifiers(void)
{
	SDL_Keymod m = SDL_GetModState();
#if (defined(__APPLE__) && defined(__MACH__))
	return ((m & KMOD_SHIFT) && (m & KMOD_CTRL)) || ((m & KMOD_ALT) && (m & KMOD_GUI));
#else
	return (m & KMOD_SHIFT) && (m & KMOD_CTRL) && !(m & KMOD_ALT) && !(m & KMOD_GUI);
#endif
}

static bool event_has_cheat_modifiers(const SDL_Event &event)
{
	Uint16 m = event.key.keysym.mod;
#if (defined(__APPLE__) && defined(__MACH__))
	return ((m & KMOD_SHIFT) && (m & KMOD_CTRL)) || ((m & KMOD_ALT) && (m & KMOD_GUI));
#else
	return (m & KMOD_SHIFT) && (m & KMOD_CTRL) && !(m & KMOD_ALT) && !(m & KMOD_GUI);
#endif
}

static void process_screen_click(const SDL_Event &event)
{
	int x = event.button.x, y = event.button.y;
#ifdef HAVE_OPENGL
	if (OGL_IsActive())
		OGL_Blitter::WindowToScreen(x, y);
#endif
	portable_process_screen_click(x, y, has_cheat_modifiers());
}

static void handle_game_key(const SDL_Event &event)
{
	SDL_Keycode key = event.key.keysym.sym;
	SDL_Scancode sc = event.key.keysym.scancode;
	bool changed_screen_mode = false;
	bool changed_prefs = false;

	if (!game_is_networked && (event.key.keysym.mod & KMOD_CTRL) && CheatsActive) {
		int type_of_cheat = process_keyword_key(key);
		if (type_of_cheat != NONE)
			handle_keyword(type_of_cheat);
	}
	if (Console::instance()->input_active()) {
		switch(key) {
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				Console::instance()->enter();
				break;
			case SDLK_ESCAPE:
				Console::instance()->abort();
				break;
			case SDLK_BACKSPACE:
				Console::instance()->backspace();
				break;
			case SDLK_DELETE:
				Console::instance()->del();
				break;
			case SDLK_UP:
				Console::instance()->up_arrow();
				break;
			case SDLK_DOWN:
				Console::instance()->down_arrow();
				break;
			case SDLK_LEFT:
				Console::instance()->left_arrow();
				break;
			case SDLK_RIGHT:
				Console::instance()->right_arrow();
				break;
			case SDLK_HOME:
				Console::instance()->line_home();
				break;
			case SDLK_END:
				Console::instance()->line_end();
				break;
			case SDLK_a:
				if (event.key.keysym.mod & KMOD_CTRL)
					Console::instance()->line_home();
				break;
			case SDLK_b:
				if (event.key.keysym.mod & KMOD_CTRL)
					Console::instance()->left_arrow();
				break;
			case SDLK_d:
				if (event.key.keysym.mod & KMOD_CTRL)
					Console::instance()->del();
				break;
			case SDLK_e:
				if (event.key.keysym.mod & KMOD_CTRL)
					Console::instance()->line_end();
				break;
			case SDLK_f:
				if (event.key.keysym.mod & KMOD_CTRL)
					Console::instance()->right_arrow();
				break;
			case SDLK_h:
				if (event.key.keysym.mod & KMOD_CTRL)
					Console::instance()->backspace();
				break;
			case SDLK_k:
				if (event.key.keysym.mod & KMOD_CTRL)
					Console::instance()->forward_clear();
				break;
			case SDLK_n:
				if (event.key.keysym.mod & KMOD_CTRL)
					Console::instance()->down_arrow();
				break;
			case SDLK_p:
				if (event.key.keysym.mod & KMOD_CTRL)
					Console::instance()->up_arrow();
				break;
			case SDLK_t:
				if (event.key.keysym.mod & KMOD_CTRL)
					Console::instance()->transpose();
				break;
			case SDLK_u:
				if (event.key.keysym.mod & KMOD_CTRL)
					Console::instance()->clear();
				break;
			case SDLK_w:
				if (event.key.keysym.mod & KMOD_CTRL)
					Console::instance()->delete_word();
				break;
		}
	}
	else
	{
		if (sc == SDL_SCANCODE_ESCAPE) // (ZZZ) Quit gesture (now safer)
		{
			if(!player_controlling_game())
				do_menu_item_command(mGame, iQuitGame, false);
			else {
				if(get_ticks_since_local_player_in_terminal() > 1 * TICKS_PER_SECOND) {
					if(!game_is_networked) {
						do_menu_item_command(mGame, iQuitGame, false);
					}
					else {
#if defined(__APPLE__) && defined(__MACH__)
						screen_printf("If you wish to quit, press Command-Q");
#else
						screen_printf("If you wish to quit, press Alt+Q.");
#endif
					}
				}
			}
		}
		else if (input_preferences->shell_key_bindings[_key_volume_up].count(sc))
		{
			changed_prefs = SoundManager::instance()->AdjustVolumeUp(Sound_AdjustVolume());
		}
		else if (input_preferences->shell_key_bindings[_key_volume_down].count(sc))
		{
			changed_prefs = SoundManager::instance()->AdjustVolumeDown(Sound_AdjustVolume());
		}
		else if (input_preferences->shell_key_bindings[_key_switch_view].count(sc))
		{
			walk_player_list();
			render_screen(NONE);
		}
		else if (input_preferences->shell_key_bindings[_key_zoom_in].count(sc))
		{
			if (zoom_overhead_map_in())
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
			else
				PlayInterfaceButtonSound(Sound_ButtonFailure());
		}
		else if (input_preferences->shell_key_bindings[_key_zoom_out].count(sc))
		{
			if (zoom_overhead_map_out())
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
			else
				PlayInterfaceButtonSound(Sound_ButtonFailure());
		}
		else if (input_preferences->shell_key_bindings[_key_inventory_left].count(sc))
		{
			if (player_controlling_game()) {
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
				scroll_inventory(-1);
			} else
				decrement_replay_speed();
		}
		else if (input_preferences->shell_key_bindings[_key_inventory_right].count(sc))
		{
			if (player_controlling_game()) {
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
				scroll_inventory(1);
			} else
				increment_replay_speed();
		}
		else if (input_preferences->shell_key_bindings[_key_toggle_fps].count(sc))
		{
			PlayInterfaceButtonSound(Sound_ButtonSuccess());
			extern bool displaying_fps;
			displaying_fps = !displaying_fps;
		}
		else if (input_preferences->shell_key_bindings[_key_activate_console].count(sc))
		{
			if (game_is_networked) {
#if !defined(DISABLE_NETWORKING)
				Console::instance()->activate_input(InGameChatCallbacks::SendChatMessage, InGameChatCallbacks::prompt());
#endif
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
			} 
			else if (Console::instance()->use_lua_console())
			{
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
				Console::instance()->activate_input(ExecuteLuaString, ">");
			}
			else
			{
				PlayInterfaceButtonSound(Sound_ButtonFailure());
			}
		} 
		else if (input_preferences->shell_key_bindings[_key_show_scores].count(sc))
		{
			PlayInterfaceButtonSound(Sound_ButtonSuccess());
			{
				extern bool ShowScores;
				ShowScores = !ShowScores;
			}
		}	
		else if (sc == SDL_SCANCODE_F1) // Decrease screen size
		{
			if (!graphics_preferences->screen_mode.hud)
			{
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
				graphics_preferences->screen_mode.hud = true;
				changed_screen_mode = changed_prefs = true;
			}
			else
			{
				int mode = alephone::Screen::instance()->FindMode(get_screen_mode()->width, get_screen_mode()->height);
				if (mode < alephone::Screen::instance()->GetModes().size() - 1)
				{
					PlayInterfaceButtonSound(Sound_ButtonSuccess());
					graphics_preferences->screen_mode.width = alephone::Screen::instance()->ModeWidth(mode + 1);
					graphics_preferences->screen_mode.height = alephone::Screen::instance()->ModeHeight(mode + 1);
					graphics_preferences->screen_mode.auto_resolution = false;
					graphics_preferences->screen_mode.hud = false;
					changed_screen_mode = changed_prefs = true;
				} else
					PlayInterfaceButtonSound(Sound_ButtonFailure());
			}
		}
		else if (sc == SDL_SCANCODE_F2) // Increase screen size
		{
			if (graphics_preferences->screen_mode.hud)
			{
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
				graphics_preferences->screen_mode.hud = false;
				changed_screen_mode = changed_prefs = true;
			}
			else
			{
				int mode = alephone::Screen::instance()->FindMode(get_screen_mode()->width, get_screen_mode()->height);
				int automode = get_screen_mode()->fullscreen ? 0 : 1;
				if (mode > automode)
				{
					PlayInterfaceButtonSound(Sound_ButtonSuccess());
					graphics_preferences->screen_mode.width = alephone::Screen::instance()->ModeWidth(mode - 1);
					graphics_preferences->screen_mode.height = alephone::Screen::instance()->ModeHeight(mode - 1);
					if ((mode - 1) == automode)
						graphics_preferences->screen_mode.auto_resolution = true;
					graphics_preferences->screen_mode.hud = true;
					changed_screen_mode = changed_prefs = true;
				} else
					PlayInterfaceButtonSound(Sound_ButtonFailure());
			}
		}
		else if (sc == SDL_SCANCODE_F3) // Resolution toggle
		{
			if (!OGL_IsActive()) {
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
				graphics_preferences->screen_mode.high_resolution = !graphics_preferences->screen_mode.high_resolution;
				changed_screen_mode = changed_prefs = true;
			} else
				PlayInterfaceButtonSound(Sound_ButtonFailure());
		}
		else if (sc == SDL_SCANCODE_F4)		// Reset OpenGL textures
		{
#ifdef HAVE_OPENGL
			if (OGL_IsActive()) {
				// Play the button sound in advance to get the full effect of the sound
				PlayInterfaceButtonSound(Sound_OGL_Reset());
				OGL_ResetTextures();
			} else
#endif
				PlayInterfaceButtonSound(Sound_ButtonInoperative());
		}
		else if (sc == SDL_SCANCODE_F5) // Make the chase cam switch sides
		{
			if (ChaseCam_IsActive())
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
			else
				PlayInterfaceButtonSound(Sound_ButtonInoperative());
			ChaseCam_SwitchSides();
		}
		else if (sc == SDL_SCANCODE_F6) // Toggle the chase cam
		{
			PlayInterfaceButtonSound(Sound_ButtonSuccess());
			ChaseCam_SetActive(!ChaseCam_IsActive());
		}
		else if (sc == SDL_SCANCODE_F7) // Toggle tunnel vision
		{
			PlayInterfaceButtonSound(Sound_ButtonSuccess());
			SetTunnelVision(!GetTunnelVision());
		}
		else if (sc == SDL_SCANCODE_F8) // Toggle the crosshairs
		{
			PlayInterfaceButtonSound(Sound_ButtonSuccess());
			player_preferences->crosshairs_active = !player_preferences->crosshairs_active;
			Crosshairs_SetActive(player_preferences->crosshairs_active);
			changed_prefs = true;
		}
		else if (sc == SDL_SCANCODE_F9) // Screen dump
		{
			dump_screen();
		}
		else if (sc == SDL_SCANCODE_F10) // Toggle the position display
		{
			PlayInterfaceButtonSound(Sound_ButtonSuccess());
			{
				extern bool ShowPosition;
				ShowPosition = !ShowPosition;
			}
		}
		else if (sc == SDL_SCANCODE_F11) // Decrease gamma level
		{
			if (graphics_preferences->screen_mode.gamma_level) {
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
				graphics_preferences->screen_mode.gamma_level--;
				change_gamma_level(graphics_preferences->screen_mode.gamma_level);
				changed_prefs = true;
			} else
				PlayInterfaceButtonSound(Sound_ButtonFailure());
		}
		else if (sc == SDL_SCANCODE_F12) // Increase gamma level
		{
			if (graphics_preferences->screen_mode.gamma_level < NUMBER_OF_GAMMA_LEVELS - 1) {
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
				graphics_preferences->screen_mode.gamma_level++;
				change_gamma_level(graphics_preferences->screen_mode.gamma_level);
				changed_prefs = true;
			} else
				PlayInterfaceButtonSound(Sound_ButtonFailure());
		}
		else
		{
			if (get_game_controller() == _demo)
				set_game_state(_close_game);
		}
	}
	
	if (changed_screen_mode) {
		screen_mode_data temp_screen_mode = graphics_preferences->screen_mode;
		temp_screen_mode.fullscreen = get_screen_mode()->fullscreen;
		change_screen_mode(&temp_screen_mode, true);
		render_screen(0);
	}

	if (changed_prefs)
		write_preferences();
}

static void process_game_key(const SDL_Event &event)
{
	switch (get_game_state()) {
	case _game_in_progress:
#if defined(__APPLE__) && defined(__MACH__)
		if ((event.key.keysym.mod & KMOD_GUI))
#else
		if ((event.key.keysym.mod & KMOD_ALT) || (event.key.keysym.mod & KMOD_GUI))
#endif
		{
			int item = -1;
			switch (event.key.keysym.sym) {
			case SDLK_p:
				item = iPause;
				break;
			case SDLK_s:
				item = iSave;
				break;
			case SDLK_r:
				item = iRevert;
				break;
// ZZZ: Alt+F4 is also a quit gesture in Windows
#ifdef __WIN32__
			case SDLK_F4:
#endif
			case SDLK_q:
				item = iQuitGame;
				break;
			case SDLK_RETURN:
				item = 0;
				toggle_fullscreen();
				break;
			default:
				break;
			}
			if (item > 0)
				do_menu_item_command(mGame, item, event_has_cheat_modifiers(event));
			else if (item != 0)
				handle_game_key(event);
		} else
			handle_game_key(event);
		break;
	case _display_intro_screens:
	case _display_chapter_heading:
	case _display_prologue:
	case _display_epilogue:
	case _display_credits:
	case _display_quit_screens:
		if (interface_fade_finished())
			force_game_state_change();
		else
			stop_interface_fade();
		break;

	case _display_intro_screens_for_demo:
		stop_interface_fade();
		display_main_menu();
		break;

	case _quit_game:
	case _close_game:
	case _revert_game:
	case _switch_demo:
	case _change_level:
	case _begin_display_of_epilogue:
	case _displaying_network_game_dialogs:
		break;

	case _display_main_menu: 
	{
		if (!interface_fade_finished())
			stop_interface_fade();
		int item = -1;
		switch (event.key.keysym.sym) {
		case SDLK_n:
			item = iNewGame;
			break;
		case SDLK_o:
			item = iLoadGame;
			break;
		case SDLK_g:
			item = iGatherGame;
			break;
		case SDLK_j:
			item = iJoinGame;
			break;
		case SDLK_p:
			item = iPreferences;
			break;
		case SDLK_r:
			item = iReplaySavedFilm;
			break;
		case SDLK_c:
			item = iCredits;
			break;
// ZZZ: Alt+F4 is also a quit gesture in Windows
#ifdef __WIN32__
                case SDLK_F4:
#endif
		case SDLK_q:
			item = iQuit;
			break;
		case SDLK_F9:
			dump_screen();
			break;
		case SDLK_RETURN:
#if defined(__APPLE__) && defined(__MACH__)
			if ((event.key.keysym.mod & KMOD_GUI))
#else
			if ((event.key.keysym.mod & KMOD_GUI) || (event.key.keysym.mod & KMOD_ALT))
#endif
			{
				toggle_fullscreen();
			}
			break;
		case SDLK_a:
			item = iAbout;
			break;
		default:
			break;
		}
		if (item > 0) {
			draw_menu_button_for_command(item);
			do_menu_item_command(mInterface, item, event_has_cheat_modifiers(event));
		}
		break;
	}
	}
}

static void process_event(const SDL_Event &event)
{
	switch (event.type) {
	case SDL_MOUSEMOTION:
		if (get_game_state() == _game_in_progress)
		{
			mouse_moved(event.motion.xrel, event.motion.yrel);
		}
		break;
	case SDL_MOUSEWHEEL:
		if (get_game_state() == _game_in_progress)
		{
			bool up = (event.wheel.y > 0);
#if SDL_VERSION_ATLEAST(2,0,4)
			if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
				up = !up;
#endif
			mouse_scroll(up);
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
		if (get_game_state() == _game_in_progress) 
		{
			if (!get_keyboard_controller_status())
			{
				hide_cursor();
				validate_world_window();
				set_keyboard_controller_status(true);
			}
			else
			{
				SDL_Event e2;
				memset(&e2, 0, sizeof(SDL_Event));
				e2.type = SDL_KEYDOWN;
				e2.key.keysym.sym = SDLK_UNKNOWN;
				e2.key.keysym.scancode = (SDL_Scancode)(AO_SCANCODE_BASE_MOUSE_BUTTON + event.button.button - 1);
				process_game_key(e2);
			}
		}
		else
			process_screen_click(event);
		break;
	
	case SDL_JOYBUTTONDOWN:
		if (get_game_state() == _game_in_progress)
		{
			SDL_Event e2;
			memset(&e2, 0, sizeof(SDL_Event));
			e2.type = SDL_KEYDOWN;
			e2.key.keysym.sym = SDLK_UNKNOWN;
			e2.key.keysym.scancode = (SDL_Scancode)(AO_SCANCODE_BASE_JOYSTICK_BUTTON + event.button.button);
			process_game_key(e2);
			
		}
		break;
		
	case SDL_KEYDOWN:
		process_game_key(event);
		break;

	case SDL_TEXTINPUT:
		if (Console::instance()->input_active()) {
		    Console::instance()->textEvent(event);
		}
		break;
		
	case SDL_QUIT:
		set_game_state(_quit_game);
		break;

	case SDL_WINDOWEVENT:
		switch (event.window.event) {
			case SDL_WINDOWEVENT_FOCUS_LOST:
				if (get_game_state() == _game_in_progress && get_keyboard_controller_status() && !Movie::instance()->IsRecording()) {
					darken_world_window();
					set_keyboard_controller_status(false);
					show_cursor();
				}
				break;
			case SDL_WINDOWEVENT_EXPOSED:
#if !defined(__APPLE__) && !defined(__MACH__) // double buffering :)
#ifdef HAVE_OPENGL
				if (MainScreenIsOpenGL())
					MainScreenSwap();
				else
#endif
					update_game_window();
#endif
				break;
		}
		break;
	}
	
}

#ifdef HAVE_PNG
extern view_data *world_view;
#endif

std::string to_alnum(const std::string& input)
{
	std::string output;
	for (std::string::const_iterator it = input.begin(); it != input.end(); ++it)
	{
		if (isalnum(*it))
		{
			output += *it;
		}
	}

	return output;
}

void dump_screen(void)
{
	// Find suitable file name
	FileSpecifier file;
	int i = 0;
	do {
		char name[256];
		const char* suffix;
#ifdef HAVE_PNG
		suffix = "png";
#else
		suffix = "bmp";
#endif
		if (get_game_state() == _game_in_progress)
		{
			sprintf(name, "%s_%04d.%s", to_alnum(static_world->level_name).c_str(), i, suffix);
		}
		else
		{
			sprintf(name, "Screenshot_%04d.%s", i, suffix);
		}

		file = screenshots_dir + name;
		i++;
	} while (file.Exists());

#ifdef HAVE_PNG
	// build some nice metadata
	std::vector<IMG_PNG_text> texts;
	std::map<std::string, std::string> metadata;

	metadata["Source"] = expand_app_variables("$appName$ $appVersion$ ($appPlatform$)");

	time_t rawtime;
	time(&rawtime);
	
	char time_string[32];
	strftime(time_string, 32,"%d %b %Y %H:%M:%S +0000", gmtime(&rawtime));
	metadata["Creation Time"] = time_string;

	if (get_game_state() == _game_in_progress)
	{
		const float FLOAT_WORLD_ONE = float(WORLD_ONE);
		const float AngleConvert = 360/float(FULL_CIRCLE);

		metadata["Level"] = static_world->level_name;

		char map_file_name[256];
		FileSpecifier fs = environment_preferences->map_file;
		fs.GetName(map_file_name);
		metadata["Map File"] = map_file_name;

		if (Scenario::instance()->GetName().size())
		{
			metadata["Scenario"] = Scenario::instance()->GetName();
		}

		metadata["Polygon"] = boost::lexical_cast<std::string>(world_view->origin_polygon_index);
		metadata["X"] = boost::lexical_cast<std::string>(world_view->origin.x / FLOAT_WORLD_ONE);
		metadata["Y"] = boost::lexical_cast<std::string>(world_view->origin.y / FLOAT_WORLD_ONE);
		metadata["Z"] = boost::lexical_cast<std::string>(world_view->origin.z / FLOAT_WORLD_ONE);
		metadata["Yaw"] = boost::lexical_cast<std::string>(world_view->yaw * AngleConvert);


		short pitch = world_view->pitch;
		if (pitch > HALF_CIRCLE) pitch -= HALF_CIRCLE;
		metadata["Pitch"] = boost::lexical_cast<std::string>(pitch * AngleConvert);
	}

	for (std::map<std::string, std::string>::const_iterator it = metadata.begin(); it != metadata.end(); ++it)
	{
		IMG_PNG_text text;
		text.key = const_cast<char*>(it->first.c_str());
		text.value = const_cast<char*>(it->second.c_str());
		texts.push_back(text);
	}

	IMG_PNG_text* textp = texts.size() ? &texts[0] : 0;
#endif

	// Without OpenGL, dumping the screen is easy
	if (!MainScreenIsOpenGL()) {
//#ifdef HAVE_PNG
//		aoIMG_SavePNG(file.GetPath(), MainScreenSurface(), IMG_COMPRESS_DEFAULT, textp, texts.size());
#ifdef HAVE_SDL_IMAGE
		IMG_SavePNG(MainScreenSurface(), file.GetPath());
#else
		SDL_SaveBMP(MainScreenSurface(), file.GetPath());
#endif
		return;
	}
	
	int video_w, video_h;
	MainScreenSize(video_w, video_h);

#ifdef HAVE_OPENGL
	// Otherwise, allocate temporary surface...
	SDL_Surface *t = SDL_CreateRGBSurface(SDL_SWSURFACE, video_w, video_h, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	  0x000000ff, 0x0000ff00, 0x00ff0000, 0);
#else
	  0x00ff0000, 0x0000ff00, 0x000000ff, 0);
#endif
	if (t == NULL)
		return;

	// ...and pixel buffer
	void *pixels = malloc(video_w * video_h * 3);
	if (pixels == NULL) {
		SDL_FreeSurface(t);
		return;
	}

	// Read OpenGL frame buffer
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, video_w, video_h, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);  // return to default

	// Copy pixel buffer (which is upside-down) to surface
	for (int y = 0; y < video_h; y++)
		memcpy((uint8 *)t->pixels + t->pitch * y, (uint8 *)pixels + video_w * 3 * (video_h - y - 1), video_w * 3);
	free(pixels);

	// Save surface
//#ifdef HAVE_PNG
//        aoIMG_SavePNG(file.GetPath(), t, IMG_COMPRESS_DEFAULT, textp, texts.size());
#ifdef HAVE_SDL_IMAGE
	IMG_SavePNG(t, file.GetPath());
#else
	SDL_SaveBMP(t, file.GetPath());
#endif
	SDL_FreeSurface(t);
#endif
}

static bool _ParseMMLDirectory(DirectorySpecifier& dir)
{
	// Get sorted list of files in directory
	vector<dir_entry> de;
	if (!dir.ReadDirectory(de))
		return false;
	sort(de.begin(), de.end());
	
	// Parse each file
	vector<dir_entry>::const_iterator i, end = de.end();
	for (i=de.begin(); i!=end; i++) {
		if (i->is_directory)
			continue;
		if (i->name[i->name.length() - 1] == '~')
			continue;
		// people stick Lua scripts in Scripts/
		if (boost::algorithm::ends_with(i->name, ".lua"))
			continue;
		
		// Construct full path name
		FileSpecifier file_name = dir + i->name;
		
		// Parse file
		ParseMMLFromFile(file_name);
	}
	
	return true;
}

void LoadBaseMMLScripts()
{
	vector <DirectorySpecifier>::const_iterator i = data_search_path.begin(), end = data_search_path.end();
	while (i != end) {
		DirectorySpecifier path = *i + "MML";
		_ParseMMLDirectory(path);
		path = *i + "Scripts";
		_ParseMMLDirectory(path);
		i++;
	}
}
			   
const char *get_application_name(void)
{
   return application_name;
}
			   
bool expand_symbolic_paths_helper(char *dest, const char *src, int maxlen, const char *symbol, DirectorySpecifier& dir)
{
   int symlen = strlen(symbol);
   if (!strncmp(src, symbol, symlen))
   {
	   strncpy(dest, dir.GetPath(), maxlen);
	   dest[maxlen] = '\0';
	   strncat(dest, &src[symlen], maxlen-strlen(dest));
	   return true;
   }
   return false;
}

char *expand_symbolic_paths(char *dest, const char *src, int maxlen)
{
	bool expanded =
#if defined(HAVE_BUNDLE_NAME)
		expand_symbolic_paths_helper(dest, src, maxlen, "$bundle$", bundle_data_dir) ||
		expand_symbolic_paths_helper(dest, src, maxlen, sBundlePlaceholder, bundle_data_dir) ||
#endif
		expand_symbolic_paths_helper(dest, src, maxlen, "$local$", local_data_dir) ||
		expand_symbolic_paths_helper(dest, src, maxlen, "$default$", default_data_dir);
	if (!expanded)
	{
		strncpy(dest, src, maxlen);
		dest[maxlen] = '\0';
	}
	return dest;
}
			   
bool contract_symbolic_paths_helper(char *dest, const char *src, int maxlen, const char *symbol, DirectorySpecifier &dir)
{
   const char *dpath = dir.GetPath();
   int dirlen = strlen(dpath);
   if (!strncmp(src, dpath, dirlen))
   {
	   strncpy(dest, symbol, maxlen);
	   dest[maxlen] = '\0';
	   strncat(dest, &src[dirlen], maxlen-strlen(dest));
	   return true;
   }
   return false;
}

char *contract_symbolic_paths(char *dest, const char *src, int maxlen)
{
	bool contracted =
#if defined(HAVE_BUNDLE_NAME)
		contract_symbolic_paths_helper(dest, src, maxlen, "$bundle$", bundle_data_dir) ||
#endif
		contract_symbolic_paths_helper(dest, src, maxlen, "$local$", local_data_dir) ||
		contract_symbolic_paths_helper(dest, src, maxlen, "$default$", default_data_dir);
	if (!contracted)
	{
		strncpy(dest, src, maxlen);
		dest[maxlen] = '\0';
	}
	return dest;
}

// LP: the rest of the code has been moved to Jeremy's shell_misc.file.

void PlayInterfaceButtonSound(short SoundID)
{
	if (TEST_FLAG(input_preferences->modifiers,_inputmod_use_button_sounds))
		SoundManager::instance()->PlaySound(SoundID, (world_location3d *) NULL, NONE);
}
