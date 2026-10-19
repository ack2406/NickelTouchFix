# NickelTouchFix

A small [NickelHook](https://github.com/pgaskin/NickelHook) mod that fixes touch input in landscape mode on Kobo e-readers where the forced landscape mode leaves touch rotated by 180°.

> [!WARNING]
> Early version, tested on one device so far. Use at your own risk. See [Status](#status).

## The problem

On devices without an orientation sensor, such as the Kobo Clara BW, landscape mode can be unlocked with:

```ini
# .kobo/Kobo/Kobo eReader.conf
[DeveloperSettings]
ForceAllowLandscape=true
```

The screen then rotates correctly, but touch input is rotated by 180°. A tap in the top-right corner registers as the bottom-left, and left and right are swapped. Switching orientation with NickelMenu (`nickel_orientation`) has the same problem. See the MobileRead thread [Clara BW landscape mode seems doesn't work correctly](https://www.mobileread.com/forums/showthread.php?t=363526).

## How it works

Kobo's touch plugin (`/usr/local/Kobo/generic/libkevdevtouch.so`) recomputes its touch transform whenever the screen orientation changes:

```cpp
setTransform(primaryScreen()->transformBetween(nativeOrientation(), orientation(), geometry));
```

NickelTouchFix hooks `QScreen::transformBetween` in that plugin's PLT. For the configured orientations, it appends a 180° rotation within the target rectangle: `(x, y)` becomes `(left + right - x, top + bottom - y)`. Rendering and other orientations are left alone. Because the fix sits at the source of the transform, it applies to taps, swipes and text selection alike.

## Installation

1. Unlock landscape mode with `ForceAllowLandscape=true` (see above).
2. Install [NickelMenu](https://github.com/pgaskin/NickelMenu) to switch orientation, for example:
   ```
   menu_item :reader :Landscape :nickel_orientation :inverted_landscape
   menu_item :reader :Portrait  :nickel_orientation :portrait
   ```
3. Download `KoboRoot.tgz` from the [releases](../../releases), copy it to the `.kobo` folder on the device, then eject and disconnect. The device installs it and reboots.

The fix applies on the next orientation change. If the device boots straight into landscape, switch to portrait and back once.

## Configuration

Optional. Create `.adds/nickeltouchfix/settings.ini`:

```ini
[TouchFix]
; Orientations in which touch is rotated by 180 degrees.
; Values: portrait, landscape, inverted_portrait, inverted_landscape
FlipOrientations=landscape, inverted_landscape
; Write orientation changes to .adds/nickeltouchfix/debug.log
Debug=false
```

Settings are read at boot. If `FlipOrientations` is not set, both landscape orientations are flipped.

## Uninstall

Delete `.adds/nickeltouchfix/uninstall` and restart the device. NickelHook's failsafe also disables the mod automatically if it crashes Nickel during startup.

## Status

| Device | Firmware | Result |
| --- | --- | --- |
| Kobo Clara BW (N365) | 4.46.23836 | Works (`inverted_landscape`) |

Reports for other devices, such as the Tolino Shine 5 or other 6" models, are welcome. Please include `debug.log` with `Debug=true`.

## Building

Builds use [NickelTC](https://github.com/pgaskin/NickelTC) through Docker:

```sh
just build
# or
docker run --rm -v "$PWD:/src" -w /src ghcr.io/pgaskin/nickeltc:1.0 make all koboroot
```

The build produces `libnickeltouchfix.so` and `KoboRoot.tgz`. GitHub Actions builds every push and creates a draft release for tags.

## Credits and license

- [NickelHook](https://github.com/pgaskin/NickelHook) by Patrick Gaskin (MIT) is vendored in `libs/NickelHook` from upstream commit `bcd90ba`.
- The project layout follows [NickelClock](https://github.com/shermp/NickelClock).

NickelTouchFix is released under the [MIT License](LICENSE). It contains no Kobo code; it hooks a public Qt function by name at runtime. Not affiliated with Rakuten Kobo.
