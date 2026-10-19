include libs/NickelHook/NickelHook.mk

override LIBRARY  := libnickeltouchfix.so
override SOURCES  += src/nickeltouchfix.cc
override CFLAGS   += -Wall -Wextra -Werror
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers

override PKGCONF  += Qt5Gui

override KOBOROOT += res/uninstall:/mnt/onboard/.adds/nickeltouchfix/uninstall

include libs/NickelHook/NickelHook.mk
