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

#include <queue>

#include <ppapi/cpp/completion_callback.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/rect.h>

#include <nacl-mounts/base/KernelProxy.h>
#include <nacl-mounts/base/MainThreadRunner.h>
#include <nacl-mounts/buffer/BufferMount.h>
#include <nacl-mounts/http2/HTTP2Mount.h>
#include <nacl-mounts/pepper/PepperMount.h>

// Code adapted from NaCl SDL tutorial and Wesnoth port.

extern "C" int uqmMain(int argc, char **argv);

class GameInstance : public pp::Instance {
 private:
  static int num_instances_;       // Ensure we only create one instance.
  pthread_t game_main_thread_;     // This thread will run game_main().
  bool sdl_initialized_;           // Ensure we initialize an instance only once.
  int width_; int height_;         // Dimension of the SDL video screen.
  pp::CompletionCallbackFactory<GameInstance> cc_factory_;

  KernelProxy* proxy_;
  MainThreadRunner* runner_;
  pp::FileSystem* fs_temporary_;
  pp::FileSystem* fs_persistent_;

  std::string manifest_;
  ssize_t manifest_size_;

  // Launches the actual game, e.g., by calling game_main().
  static void* LaunchGameThunk(void* data);
  void LaunchGame();

  class JSDirectoryReader : public DirectoryReader {
  public:
    JSDirectoryReader(pp::Instance* instance) : instance_(instance) { }

    virtual int ReadDirectory(const std::string& path,
			      std::set<std::string>* entries,
			      const pp::CompletionCallback& cc) {
      DirectoryRequest req = { path, entries, cc };
      requests_.push(req);
      std::string message = "ReadDirectory";
      message += '\0'; message += path;
      instance_->PostMessage(message);
      return 0;
    }

    void HandleResponse(const std::string& response) {
      assert(!requests_.empty());
      DirectoryRequest req = requests_.front(); requests_.pop();
      if (strcmp(response.c_str(), "OK") != 0) {
	fprintf(stderr, "ReadDirectory failed (%s)\n", response.c_str());
	req.cc.Run(PP_ERROR_FAILED);
	return;
      }

      size_t pos = response.find('\0');
      while (pos != std::string::npos) {
	// Add the string starting at pos, via gross abuse of C
	// NUL-terminated strings.
	req.entries->insert(response.c_str() + pos + 1);
	pos = response.find('\0', pos + 1);
      }
      req.cc.Run(PP_OK);
    }

  private:
    // For simplicity, process these in order. Probably safe to even
    // assume only one thread does file I/O, but better to be correct.
    struct DirectoryRequest {
      std::string path;
      std::set<std::string>* entries;
      pp::CompletionCallback cc;
    };
    std::queue<DirectoryRequest> requests_;
    pp::Instance* instance_;
  };
  JSDirectoryReader directory_reader_;

  class JSProgressHandler : public HTTP2ProgressHandler {
  public:
    JSProgressHandler(pp::Instance* instance) : instance_(instance) { }

    virtual void HandleProgress(std::string& path,
				int64_t bytes,
				int64_t size) {
      std::string message = "HandleProgress";
      message += '\0'; message += path;
      char buf[30];
      snprintf(buf, sizeof(buf), "%lld", bytes);
      message += '\0'; message += buf;
      snprintf(buf, sizeof(buf), "%lld", size);
      message += '\0'; message += buf;
      instance_->PostMessage(message);
    }

  private:
    pp::Instance* instance_;
  };
  JSProgressHandler progress_handler_;

 public:

  explicit GameInstance(PP_Instance instance)
    : pp::Instance(instance),
      game_main_thread_(NULL),
      sdl_initialized_(false),
      width_(0), height_(0),
      cc_factory_(this),
      manifest_size_(-1),
      directory_reader_(this),
      progress_handler_(this) {
    // Game requires mouse and keyboard events; add more if necessary.
    RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE |
                       PP_INPUTEVENT_CLASS_KEYBOARD);
    ++num_instances_;
    assert (num_instances_ == 1);

    proxy_ = KernelProxy::KPInstance();
    runner_ = new MainThreadRunner(this);

    fs_temporary_ = new pp::FileSystem(this, PP_FILESYSTEMTYPE_LOCALTEMPORARY);
    fs_persistent_ = new pp::FileSystem(this, PP_FILESYSTEMTYPE_LOCALPERSISTENT);
  }

  virtual ~GameInstance() {
    // Wait for game thread to finish.
    if (game_main_thread_) { pthread_join(game_main_thread_, NULL); }

    // Clean up stuff.
    delete runner_;
    delete fs_temporary_;
    delete fs_persistent_;
  }

  // This function is called with the HTML attributes of the embed tag,
  // which can be used in lieu of command line arguments.
  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
    for (uint32_t i = 0; i < argc; i++) {
      if (strcmp(argn[i], "manifest") == 0) {
	manifest_ = argv[i];
      }
      if (strcmp(argn[i], "manifestsize") == 0) {
	char *end;
	manifest_size_ = strtol(argv[i], &end, 10);
	if (*argv[i] == '\0' || *end != '\0') {
	  fprintf(stderr, "manifestsize '%s' is not an integer\n", argv[i]);
	  return false;
	}
      }
    }
    if (manifest_.empty()) {
      fprintf(stderr, "Manifest missing\n");
      return false;
    }
    fprintf(stderr, "manifest_size_ = %d\n", manifest_size_);
    return true;
  }

  // This crucial function forwards PPAPI events to SDL.
  virtual bool HandleInputEvent(const pp::InputEvent& event) {
    SDL_NACL_PushEvent(event);
    return true;
  }

  virtual void HandleMessage(const pp::Var& message) {
    std::string s = message.AsString();
    size_t pos = s.find('\0');
    if (pos != std::string::npos && strcmp(s.c_str(), "ReadDirectory") == 0) {
      directory_reader_.HandleResponse(s.substr(pos + 1));
    } else {
      fprintf(stderr, "Unexpected message\n");
    }
  }

  virtual void DidChangeFocus(bool has_focus) {
    SDL_NACL_SetHasFocus(has_focus);
  }

  // This function is called for various reasons, e.g. visibility and page
  // size changes. We ignore these calls except for the first
  // invocation, which we use to start the game.
  virtual void DidChangeView(const pp::View& view) {
    if (!sdl_initialized_) {
      sdl_initialized_ = true;

      width_ = view.GetRect().size().width();
      height_ = view.GetRect().size().height();

      // NOTE: It is crucial that the two calls below are run here
      // and not in a thread.
      SDL_NACL_SetInstance(pp_instance(), width_, height_);
      // This is SDL_Init call which used to be in game_main()
      SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
      // Initialize audio and joystick here, not on the game thread.
      SDL_InitSubSystem(SDL_INIT_AUDIO);
      SDL_InitSubSystem(SDL_INIT_JOYSTICK);

      // And launch the game.
      pthread_create(&game_main_thread_, NULL, &LaunchGameThunk, this);
    }
    SDL_NACL_SetPageVisible(view.IsPageVisible());
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

// static
void* GameInstance::LaunchGameThunk(void* data) {
  static_cast<GameInstance*>(data)->LaunchGame();
  return NULL;
}

void GameInstance::LaunchGame() {
  // Setup filesystem.

  // Setup writable homedir.
  PepperMount* pepper_mount = new PepperMount(runner_, fs_persistent_,
					      2 * 1024 * 1024);
  pepper_mount->SetDirectoryReader(&directory_reader_);
  pepper_mount->SetPathPrefix("/userdata");

  proxy_->mkdir("/userdata", 0777);
  int res = proxy_->mount("/userdata", pepper_mount);
  if (!res) {
    fprintf(stderr, "/userdata initialization success.\n");
  } else {
    fprintf(stderr, "/userdata initialization failure.\n");
  }

  // Setup r/o data directory in /content
  HTTP2Mount* http2_mount = new HTTP2Mount(runner_, "./data");
  // FIXME: upgrades??
  http2_mount->SetLocalCache(fs_temporary_, 350*1024*1024, "/content-cache");
  http2_mount->SetProgressHandler(&progress_handler_);

  http2_mount->ReadManifest(manifest_, manifest_size_);

  proxy_->mkdir("/content", 0777);
  res = proxy_->mount("/content", new BufferMount(http2_mount, 100 * 1024, 1000));
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
