/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "port.h"

#include <assert.h>
#include SDL_INCLUDE(SDL.h)
#include SDL_INCLUDE(SDL_nacl.h)

#include <ppapi/cpp/completion_callback.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>

class GameInstance : public pp::Instance {
 private:
  static int num_instances_;       // Ensure we only create one instance.
  pthread_t game_main_thread_;     // This thread will run game_main().
  int num_changed_view_;           // Ensure we initialize an instance only once.
  int width_; int height_;         // Dimension of the SDL video screen.
  pp::CompletionCallbackFactory<GameInstance> cc_factory_;

  // Launches the actual game, e.g., by calling game_main().
  static void* LaunchGame(void* data);

  // This function allows us to delay game start until all
  // resources are ready.
  void StartGameInNewThread(int32_t dummy);

 public:

  explicit GameInstance(PP_Instance instance)
    : pp::Instance(instance),
      game_main_thread_(NULL),
      num_changed_view_(0),
      width_(0), height_(0),
      cc_factory_(this) {
    // Game requires mouse and keyboard events; add more if necessary.
    RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE|
                       PP_INPUTEVENT_CLASS_KEYBOARD);
    ++num_instances_;
    assert (num_instances_ == 1);
  }

  virtual ~GameInstance() {
    // Wait for game thread to finish.
    if (game_main_thread_) { pthread_join(game_main_thread_, NULL); }
  }

  // This function is called with the HTML attributes of the embed tag,
  // which can be used in lieu of command line arguments.
  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
    // [Process arguments and set width_ and height_]
    // [Initiate the loading of resources]
    return true;
  }

  // This crucial function forwards PPAPI events to SDL.
  virtual bool HandleInputEvent(const pp::InputEvent& event) {
    SDL_NACL_PushEvent(event);
    return true;
  }

  // This function is called for various reasons, e.g. visibility and page
  // size changes. We ignore these calls except for the first
  // invocation, which we use to start the game.
  virtual void DidChangeView(const pp::Rect& position, const pp::Rect& clip) {
    ++num_changed_view_;
    if (num_changed_view_ > 1) return;
    // NOTE: It is crucial that the two calls below are run here
    // and not in a thread.
    SDL_NACL_SetInstance(pp_instance(), width_, height_);
    // This is SDL_Init call which used to be in game_main()
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
    StartGameInNewThread(0);
  }
}; 

class GameModule : public pp::Module {
 public:
  GameModule() : pp::Module() {}
  virtual ~GameModule() {}

  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new GameInstance(instance);
  }
};

// static
int GameInstance::num_instances_;

void GameInstance::StartGameInNewThread(int32_t dummy) {
  if (true /* [All Resources Are Ready] */) {
    pthread_create(&game_main_thread_, NULL, &LaunchGame, this);
  } else {
    // Wait some more (here: 100ms).
    pp::Module::Get()->core()->CallOnMainThread(
        100, cc_factory_.NewCallback(&GameInstance::StartGameInNewThread), 0);
  }
}

// static
void* GameInstance::LaunchGame(void* data) {
#if 0
  // Use "thiz" to get access to instance object.
  GameInstance* thiz = reinterpret_cast(data);
  // Craft a fake command line.
  const char* argv[] = { "game",  ...   };
  game_main(sizeof(argv) / sizeof(argv[0]), argv);
#endif
  assert(false);
  return 0;
}

namespace pp {
Module* CreateModule() {
  return new GameModule();
}
}  // namespace pp
