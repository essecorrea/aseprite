// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/engine.h"

#include "app/app.h"
#include "app/console.h"
#include "app/script/luacpp.h"
#include "base/chrono.h"
#include "base/fs.h"
#include "base/fstream_path.h"
#include "doc/anidir.h"
#include "doc/blend_mode.h"
#include "doc/color_mode.h"

#include <fstream>
#include <sstream>

namespace app {
namespace script {

namespace {

// High precision clock.
base::Chrono luaClock;

int print(lua_State* L)
{
  std::string output;
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    lua_pushvalue(L, -1);  // function to be called
    lua_pushvalue(L, i);   // value to print
    lua_call(L, 1, 1);
    size_t l;
    const char* s = lua_tolstring(L, -1, &l);  // get result
    if (s == nullptr)
      return luaL_error(L, "'tostring' must return a string to 'print'");
    if (i > 1)
      output.push_back('\t');
    output.insert(output.size(), s, l);
    lua_pop(L, 1);  // pop result
  }
  if (!output.empty()) {
    std::printf("%s\n", output.c_str());
    std::fflush(stdout);
    if (App::instance()->isGui())
      Console().printf("%s\n", output.c_str());
  }
  return 0;
}

int os_clock(lua_State* L)
{
  lua_pushnumber(L, luaClock.elapsed());
  return 1;
}

int unsupported(lua_State* L)
{
  // debug.getinfo(1, "n").name
  lua_getglobal(L, "debug");
  lua_getfield(L, -1, "getinfo");
  lua_remove(L, -2);
  lua_pushinteger(L, 1);
  lua_pushstring(L, "n");
  lua_call(L, 2, 1);
  lua_getfield(L, -1, "name");
  return luaL_error(L, "unsupported function '%s'",
                    lua_tostring(L, -1));
}

} // anonymous namespace

void register_app_object(lua_State* L);
void register_app_pixel_color_object(lua_State* L);
void register_app_command_object(lua_State* L);

void register_cel_class(lua_State* L);
void register_cels_class(lua_State* L);
void register_color_class(lua_State* L);
void register_frame_class(lua_State* L);
void register_frames_class(lua_State* L);
void register_image_class(lua_State* L);
void register_image_iterator_class(lua_State* L);
void register_image_spec_class(lua_State* L);
void register_layer_class(lua_State* L);
void register_layers_class(lua_State* L);
void register_palette_class(lua_State* L);
void register_palettes_class(lua_State* L);
void register_point_class(lua_State* L);
void register_rect_class(lua_State* L);
void register_selection_class(lua_State* L);
void register_site_class(lua_State* L);
void register_size_class(lua_State* L);
void register_slice_class(lua_State* L);
void register_slices_class(lua_State* L);
void register_sprite_class(lua_State* L);
void register_tag_class(lua_State* L);
void register_tags_class(lua_State* L);

Engine::Engine()
  : L(luaL_newstate())
  , m_delegate(nullptr)
  , m_printLastResult(false)
{
#if _DEBUG
  int top = lua_gettop(L);
#endif

  // Standard Lua libraries
  luaL_requiref(L, LUA_GNAME, luaopen_base, 1);
  luaL_requiref(L, LUA_COLIBNAME, luaopen_coroutine, 1);
  luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
  luaL_requiref(L, LUA_OSLIBNAME, luaopen_os, 1);
  luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
  luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
  luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1);
  luaL_requiref(L, LUA_DBLIBNAME, luaopen_debug, 1);
  lua_pop(L, 8);

  // Overwrite Lua functions
  lua_register(L, "print", print);

  lua_getglobal(L, "os");
  for (const char* name : { "execute", "remove", "rename", "exit", "tmpname" }) {
    lua_pushcfunction(L, unsupported);
    lua_setfield(L, -2, name);
  }
  lua_pushcfunction(L, os_clock);
  lua_setfield(L, -2, "clock");
  lua_pop(L, 1);

  // Generic code used by metatables
  run_mt_index_code(L);

  // Register global app object
  register_app_object(L);
  register_app_pixel_color_object(L);
  register_app_command_object(L);

  // Register constants
  lua_newtable(L);
  lua_pushvalue(L, -1);
  lua_setglobal(L, "ColorMode");
  setfield_integer(L, "RGB", doc::ColorMode::RGB);
  setfield_integer(L, "GRAYSCALE", doc::ColorMode::GRAYSCALE);
  setfield_integer(L, "INDEXED", doc::ColorMode::INDEXED);
  lua_pop(L, 1);

  lua_newtable(L);
  lua_pushvalue(L, -1);
  lua_setglobal(L, "AniDir");
  setfield_integer(L, "FORWARD", doc::AniDir::FORWARD);
  setfield_integer(L, "REVERSE", doc::AniDir::REVERSE);
  setfield_integer(L, "PING_PONG", doc::AniDir::PING_PONG);
  lua_pop(L, 1);

  lua_newtable(L);
  lua_pushvalue(L, -1);
  lua_setglobal(L, "BlendMode");
  setfield_integer(L, "NORMAL", doc::BlendMode::NORMAL);
  setfield_integer(L, "MULTIPLY", doc::BlendMode::MULTIPLY);
  setfield_integer(L, "SCREEN", doc::BlendMode::SCREEN);
  setfield_integer(L, "OVERLAY", doc::BlendMode::OVERLAY);
  setfield_integer(L, "DARKEN", doc::BlendMode::DARKEN);
  setfield_integer(L, "LIGHTEN", doc::BlendMode::LIGHTEN);
  setfield_integer(L, "COLOR_DODGE", doc::BlendMode::COLOR_DODGE);
  setfield_integer(L, "COLOR_BURN", doc::BlendMode::COLOR_BURN);
  setfield_integer(L, "HARD_LIGHT", doc::BlendMode::HARD_LIGHT);
  setfield_integer(L, "SOFT_LIGHT", doc::BlendMode::SOFT_LIGHT);
  setfield_integer(L, "DIFFERENCE", doc::BlendMode::DIFFERENCE);
  setfield_integer(L, "EXCLUSION", doc::BlendMode::EXCLUSION);
  setfield_integer(L, "HSL_HUE", doc::BlendMode::HSL_HUE);
  setfield_integer(L, "HSL_SATURATION", doc::BlendMode::HSL_SATURATION);
  setfield_integer(L, "HSL_COLOR", doc::BlendMode::HSL_COLOR);
  setfield_integer(L, "HSL_LUMINOSITY", doc::BlendMode::HSL_LUMINOSITY);
  setfield_integer(L, "ADDITION", doc::BlendMode::ADDITION);
  setfield_integer(L, "SUBTRACT", doc::BlendMode::SUBTRACT);
  setfield_integer(L, "DIVIDE", doc::BlendMode::DIVIDE);
  lua_pop(L, 1);

  // Register classes/prototypes
  register_cel_class(L);
  register_cels_class(L);
  register_color_class(L);
  register_frame_class(L);
  register_frames_class(L);
  register_image_class(L);
  register_image_iterator_class(L);
  register_image_spec_class(L);
  register_layer_class(L);
  register_layers_class(L);
  register_palette_class(L);
  register_palettes_class(L);
  register_point_class(L);
  register_rect_class(L);
  register_selection_class(L);
  register_site_class(L);
  register_size_class(L);
  register_slice_class(L);
  register_slices_class(L);
  register_sprite_class(L);
  register_tag_class(L);
  register_tags_class(L);

  // Check that we have a clean start (without dirty in the stack)
  ASSERT(lua_gettop(L) == top);
}

Engine::~Engine()
{
  lua_close(L);
}

void Engine::printLastResult()
{
  m_printLastResult = true;
}

bool Engine::evalCode(const std::string& code,
                      const std::string& filename)
{
  bool ok = true;
  if (luaL_loadbuffer(L, code.c_str(), code.size(), filename.c_str()) ||
      lua_pcall(L, 0, 1, 0)) {
    // Error case
    std::string err;
    const char* s = lua_tostring(L, -1);
    if (s)
      onConsolePrint(s);
    ok = false;
  }
  else {
    // Code was executed correctly
    if (m_printLastResult) {
      if (!lua_isnone(L, -1)) {
        const char* result = lua_tostring(L, -1);
        if (result)
          onConsolePrint(result);
      }
    }
  }
  lua_pop(L, 1);
  return ok;
}

bool Engine::evalFile(const std::string& filename)
{
  std::stringstream buf;
  {
    std::ifstream s(FSTREAM_PATH(filename));
    buf << s.rdbuf();
  }

  std::string fn = filename;
  if (fn.size() > 2 &&
#ifdef _WIN32
      fn[1] != ':'
#else
      fn[0] != '/'
#endif
      ) {
    fn = base::join_path(base::get_current_path(), fn);
  }
  fn = base::get_canonical_path(fn);
  return evalCode(buf.str(), "@" + fn);
}

void Engine::onConsolePrint(const char* text)
{
  if (m_delegate)
    m_delegate->onConsolePrint(text);
  else
    std::printf("%s\n", text);
}

} // namespace script
} // namespace app
