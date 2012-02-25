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
#include <ppapi/cpp/rect.h>

#include <nacl-mounts/base/KernelProxy.h>
#include <nacl-mounts/base/MainThreadRunner.h>
#include <nacl-mounts/http2/HTTP2Mount.h>
#include <nacl-mounts/pepper/PepperMount.h>

extern "C" int uqmMain(int argc, char **argv);

class GameInstance : public pp::Instance {
 private:
  static int num_instances_;       // Ensure we only create one instance.
  pthread_t game_main_thread_;     // This thread will run game_main().
  int num_changed_view_;           // Ensure we initialize an instance only once.
  int width_; int height_;         // Dimension of the SDL video screen.
  pp::CompletionCallbackFactory<GameInstance> cc_factory_;

  KernelProxy* proxy_;
  MainThreadRunner* runner_;
  pp::FileSystem* fs_;

  // Launches the actual game, e.g., by calling game_main().
  static void* LaunchGameThunk(void* data);
  void LaunchGame();

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
    RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE |
                       PP_INPUTEVENT_CLASS_KEYBOARD);
    ++num_instances_;
    assert (num_instances_ == 1);

    proxy_ = KernelProxy::KPInstance();
    runner_ = new MainThreadRunner(this);

    // Persistent seems to hate me?
    fprintf(stderr, "Requesting an HTML5 local tepmorary filesystem.\n");
    fs_ = new pp::FileSystem(this, PP_FILESYSTEMTYPE_LOCALTEMPORARY);
  }

  virtual ~GameInstance() {
    // Wait for game thread to finish.
    if (game_main_thread_) { pthread_join(game_main_thread_, NULL); }

    // Clean up stuff.
    delete runner_;
    delete fs_;
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

    width_ = position.size().width();
    height_ = position.size().height();

    // NOTE: It is crucial that the two calls below are run here
    // and not in a thread.
    SDL_NACL_SetInstance(pp_instance(), width_, height_);
    // This is SDL_Init call which used to be in game_main()
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
    // Initialize audio and joystick here, not on the game thread.
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
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
    pthread_create(&game_main_thread_, NULL, &LaunchGameThunk, this);
  } else {
    // Wait some more (here: 100ms).
    pp::Module::Get()->core()->CallOnMainThread(
        100, cc_factory_.NewCallback(&GameInstance::StartGameInNewThread), 0);
  }
}

// static
void* GameInstance::LaunchGameThunk(void* data) {
  // Use "thiz" to get access to instance object.
  GameInstance* thiz = static_cast<GameInstance*>(data);
  thiz->LaunchGame();
  return NULL;
}

void GameInstance::LaunchGame() {
  // Setup filesystem.

  // Setup writable homedir.
  PepperMount* pepper_mount = new PepperMount(runner_, fs_, 20 * 1024 * 1024);
  //  pepper_mount->SetDirectoryReader(&directory_reader_);
  pepper_mount->SetPathPrefix("/userdata");

  proxy_->mkdir("/userdata", 0777);
  int res = proxy_->mount("/userdata", pepper_mount);
  if (!res) {
    fprintf(stderr, "/userdata initialization success.\n");
  } else {
    fprintf(stderr, "/userdata initialization failure.\n");
  }

  // Setup r/o data directory in /content
  HTTP2Mount* http2_mount = new HTTP2Mount(runner_, "/data/0.7.0");
  // FIXME: upgrades??
  http2_mount->SetLocalCache(fs_, 350*1024*1024, "/content-cache", true);
  //  http2_mount->SetProgressHandler(&progress_handler_);

  http2_mount->ReadManifest("/manifest.0aaf28752056d176a94398ad6608940c33a44978");

  // FIXME: nacl-mounts is buggy and can't readdir the root of an
  // HTTP2 mount. Probably should workaround this outside the library
  // too, to ease building.
  proxy_->mkdir("/content", 0777);
  res = proxy_->mount("/content", http2_mount);
  if (!res) {
    fprintf(stderr, "/content initialization success.\n");
  } else {
    fprintf(stderr, "/content initialization failure.\n");
  }

  // Craft a fake command line. We can probably compile-in some of
  // this directory options, but whatever.
  char buf[100];
  snprintf(buf, sizeof(buf), "%dx%d", width_, height_);
  const char* argv[] = {
    "uqm",
    "--res", buf,
    "--configdir", "/userdata",
    "--contentdir", "/content",
    NULL
  };
  uqmMain(sizeof(argv) / sizeof(argv[0]) - 1, const_cast<char**>(argv));
}

namespace pp {
Module* CreateModule() {
  return new GameModule();
}
}  // namespace pp
