#include <libinput.h>
#include <libudev.h>
#include <xkbcommon/xkbcommon.h>

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
static int open_restricted(const char *path, int flags, void *user_data) {
    int fd = open(path, flags);
    if (fd < 0) {
        std::cerr << "Failed to open: " << path << "\n";
    }
    return fd;
}

static void close_restricted(int fd, void *user_data) {
    close(fd);
}

const struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};

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
    xkb_keymap *keymap = xkb_keymap_new_from_names(ctx, nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS);
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
                xkb_state_update_key(
                    xkbState,
                    key + 8, // IMPORTANT offset
                    state == LIBINPUT_KEY_STATE_PRESSED ? XKB_KEY_DOWN : XKB_KEY_UP
                );

                if (state == LIBINPUT_KEY_STATE_PRESSED) {
                    xkb_keysym_t sym = xkb_state_key_get_one_sym(xkbState, key + 8);

                    char name[64];
                    xkb_keysym_get_name(sym, name, sizeof(name));

                    std::cout << "Key pressed: " << name << std::endl;
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
