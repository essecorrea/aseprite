// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPTING_H_INCLUDED
#define APP_SCRIPTING_H_INCLUDED
#pragma once

#ifndef ENABLE_SCRIPTING
  #error ENABLE_SCRIPTING must be defined
#endif

#include "app/color.h"
#include "doc/frame.h"
#include "doc/image_ref.h"
#include "doc/object_id.h"
#include "gfx/fwd.h"

#include <cstdio>
#include <string>
#include <map>

struct lua_State;

namespace doc {
  class Cel;
  class FrameTag;
  class Layer;
  class Palette;
  class Sprite;
  class WithUserData;
}

namespace app {
  class Site;
  namespace script {

  class EngineDelegate {
  public:
    virtual ~EngineDelegate() { }
    virtual void onConsolePrint(const char* text) = 0;
  };

  class Engine {
  public:
    Engine();
    ~Engine();

    EngineDelegate* delegate() { return m_delegate; }
    void setDelegate(EngineDelegate* delegate) {
      m_delegate = delegate;
    }

    void printLastResult();
    bool evalCode(const std::string& code,
                  const std::string& filename = std::string());
    bool evalFile(const std::string& filename);

  private:
    void onConsolePrint(const char* text);

    lua_State* L;
    EngineDelegate* m_delegate;
    bool m_printLastResult;
  };

  class ScopedEngineDelegate {
  public:
    ScopedEngineDelegate(Engine* engine, EngineDelegate* delegate)
      : m_engine(engine),
        m_oldDelegate(engine->delegate()) {
      m_engine->setDelegate(delegate);
    }
    ~ScopedEngineDelegate() {
      m_engine->setDelegate(m_oldDelegate);
    }
  private:
    Engine* m_engine;
    EngineDelegate* m_oldDelegate;
  };

  int push_image_iterator_function(lua_State* L, const doc::ImageRef& image, int extraArgIndex);
  void push_cel_image(lua_State* L, doc::Cel* cel);
  void push_sprite_cel(lua_State* L, doc::Cel* cel);
  void push_sprite_cels(lua_State* L, doc::Sprite* sprite);
  void push_layer_cels(lua_State* L, doc::Layer* layer);
  void push_sprite_frame(lua_State* L, doc::Sprite* sprite, doc::frame_t frame);
  void push_sprite_frames(lua_State* L, doc::Sprite* sprite);
  void push_sprite_layers(lua_State* L, doc::Sprite* sprite);
  void push_sprite_palette(lua_State* L, doc::Sprite* sprite, doc::Palette* palette);
  void push_sprite_palettes(lua_State* L, doc::Sprite* sprite);
  void push_sprite_selection(lua_State* L, doc::Sprite* sprite);
  void push_sprite_slices(lua_State* L, doc::Sprite* sprite);
  void push_sprite_tags(lua_State* L, doc::Sprite* sprite);
  void push_userdata(lua_State* L, doc::WithUserData* userData);

  gfx::Point convert_args_into_point(lua_State* L, int index);
  gfx::Rect convert_args_into_rect(lua_State* L, int index);
  gfx::Size convert_args_into_size(lua_State* L, int index);
  app::Color convert_args_into_color(lua_State* L, int index);
  doc::color_t convert_args_into_pixel_color(lua_State* L, int index);
  doc::Palette* get_palette_from_arg(lua_State* L, int index);
  doc::Image* may_get_image_from_arg(lua_State* L, int index);

} // namespace script
} // namespace app

#endif
