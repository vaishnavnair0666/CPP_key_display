#include <libinput.h>
#include <libudev.h>
#include <xkbcommon/xkbcommon.h>

#include <fcntl.h>
#include <iostream>
#include <unistd.h>

#include <set>
#include <sstream>
#include <string>

static int open_restricted(const char *path, int flags, void *user_data) {
  int fd = open(path, flags);
  if (fd < 0) {
    std::cerr << "Failed to open: " << path << "\n";
  }
  return fd;
}

static void close_restricted(int fd, void *user_data) { close(fd); }

const struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};

std::set<std::string> activeKeys;

#include <algorithm>
#include <unordered_map>
#include <vector>
static std::unordered_map<std::string, int> keyPriority = {
    {"CTRL", 1},
    {"SHIFT", 2},
    {"ALT", 3},
    {"SUPER", 4},
};

std::string format_keys(const std::set<std::string> &keys) {
  std::vector<std::string> sorted(keys.begin(), keys.end());

  std::sort(sorted.begin(), sorted.end(),
            [](const std::string &a, const std::string &b) {
              int pa = keyPriority.count(a) ? keyPriority[a] : 100;
              int pb = keyPriority.count(b) ? keyPriority[b] : 100;

              if (pa != pb)
                return pa < pb;

              return a < b; // fallback alphabetical
            });

  std::ostringstream oss;
  for (size_t i = 0; i < sorted.size(); ++i) {
    if (i > 0)
      oss << " + ";
    oss << sorted[i];
  }

  return oss.str();
}

std::string normalize_key(const std::string &key) {
  static std::unordered_map<std::string, std::string> map = {
      // Modifiers
      {"Control_L", "CTRL"},
      {"Control_R", "CTRL"},
      {"Shift_L", "SHIFT"},
      {"Shift_R", "SHIFT"},
      {"Alt_L", "ALT"},
      {"Alt_R", "ALT"},
      {"Super_L", "SUPER"},
      {"Super_R", "SUPER"},

      // Common keys
      {"Return", "ENTER"},
      {"Escape", "ESC"},
      {"BackSpace", "BACKSPACE"},
      {"Tab", "TAB"},
      {"space", "SPACE"},

      // Arrow keys
      {"Up", "UP"},
      {"Down", "DOWN"},
      {"Left", "LEFT"},
      {"Right", "RIGHT"},

      // Delete / Insert
      {"Delete", "DEL"},
      {"Insert", "INS"},
      {"Home", "HOME"},
      {"End", "END"},
      {"Page_Up", "PGUP"},
      {"Page_Down", "PGDN"},

      // Keypad cleanup
      {"KP_Enter", "ENTER"},
      {"KP_Add", "+"},
      {"KP_Subtract", "-"},
      {"KP_Multiply", "*"},
      {"KP_Divide", "/"},
  };

  // Direct match
  auto it = map.find(key);
  if (it != map.end()) {
    return it->second;
  }

  // Handle KP_ prefix generically
  if (key.rfind("KP_", 0) == 0) {
    return key.substr(3); // remove "KP_"
  }

  // Default: uppercase everything
  std::string result = key;
  for (auto &c : result)
    c = toupper(c);

  return result;
}

int main() {
  struct udev *udev = udev_new();
  if (!udev) {
    std::cerr << "Failed to create udev\n";
    return 1;
  }

  struct libinput *li = libinput_udev_create_context(&interface, nullptr, udev);
  if (!li) {
    std::cerr << "Failed to create libinput context\n";
    return 1;
  }

  if (libinput_udev_assign_seat(li, "seat0") != 0) {
    std::cerr << "Failed to assign seat\n";
    return 1;
  }

  xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  xkb_keymap *keymap =
      xkb_keymap_new_from_names(ctx, nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS);
  xkb_state *xkbState = xkb_state_new(keymap);

  std::cout << "Listening for key events...\n";

  while (true) {
    libinput_dispatch(li);

    struct libinput_event *event;
    while ((event = libinput_get_event(li)) != nullptr) {

      if (libinput_event_get_type(event) == LIBINPUT_EVENT_KEYBOARD_KEY) {
        auto *kbevent = libinput_event_get_keyboard_event(event);

        uint32_t key = libinput_event_keyboard_get_key(kbevent);
        auto state = libinput_event_keyboard_get_key_state(kbevent);

        // Update xkb state
        xkb_state_update_key(xkbState,
                             key + 8, // IMPORTANT offset
                             state == LIBINPUT_KEY_STATE_PRESSED ? XKB_KEY_DOWN
                                                                 : XKB_KEY_UP);

        xkb_keysym_t sym = xkb_state_key_get_one_sym(xkbState, key + 8);

        char name[64];
        xkb_keysym_get_name(sym, name, sizeof(name));

        std::string keyName = normalize_key(name);

        if (state == LIBINPUT_KEY_STATE_PRESSED) {
          activeKeys.insert(keyName);

          std::cout << "Combo: " << format_keys(activeKeys) << std::endl;
        } else if (state == LIBINPUT_KEY_STATE_RELEASED) {
          activeKeys.erase(keyName);
        }
      }

      libinput_event_destroy(event);
    }

    usleep(1000); // prevent 100% CPU
  }

  // Cleanup
  libinput_unref(li);
  udev_unref(udev);
}
