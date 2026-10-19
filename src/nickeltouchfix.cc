// NickelTouchFix - corrects touch input in landscape on Kobo devices where the
// forced landscape mode (ForceAllowLandscape) leaves touch rotated by 180°
// (observed on the Kobo Clara BW, firmware 4.46).
//
// Kobo's touch plugin (generic/libkevdevtouch.so) recomputes its touch
// transform on orientation changes with:
//
//   setTransform(primaryScreen()->transformBetween(nativeOrientation(), orientation(), geometry))
//
// We hook QScreen::transformBetween in that plugin's PLT and, for the
// configured orientations, append a 180° rotation within the target rect.

#include <cstddef>
#include <cstdlib>

#include <QDateTime>
#include <QFile>
#include <QGuiApplication>
#include <QRect>
#include <QScreen>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QTransform>

#include <NickelHook.h>

#define NTF_DIR "/mnt/onboard/.adds/nickeltouchfix"
#define NTF_TOUCH_LIB "/usr/local/Kobo/generic/libkevdevtouch.so"

typedef QTransform (*transformBetween_t)(QScreen const *self, Qt::ScreenOrientation a, Qt::ScreenOrientation b, QRect const &target);

static transformBetween_t QScreen__transformBetween;

static int ntf_flip_mask = Qt::LandscapeOrientation | Qt::InvertedLandscapeOrientation;
static bool ntf_debug = false;

static struct nh_info NickelTouchFix = {
    .name            = "NickelTouchFix",
    .desc            = "Fixes 180° rotated touch input in landscape",
    .uninstall_flag  = nullptr,
    .uninstall_xflag = NTF_DIR "/uninstall",
    .failsafe_delay  = 10,
};

static struct nh_hook NickelTouchFixHook[] = {
    {
        .sym     = "_ZNK7QScreen16transformBetweenEN2Qt17ScreenOrientationES1_RK5QRect",
        .sym_new = "_ntf_transform_between",
        .lib     = NTF_TOUCH_LIB,
        .out     = nh_symoutptr(QScreen__transformBetween),
        .desc    = "touch transform on orientation change",
        .optional = true,
    },
    {0},
};

static struct nh_dlsym NickelTouchFixDlsym[] = {
    {0},
};

static const char *ntf_orientation_name(int o) {
    switch (o) {
    case Qt::PrimaryOrientation:           return "primary";
    case Qt::PortraitOrientation:          return "portrait";
    case Qt::LandscapeOrientation:         return "landscape";
    case Qt::InvertedPortraitOrientation:  return "inverted_portrait";
    case Qt::InvertedLandscapeOrientation: return "inverted_landscape";
    default:                               return "unknown";
    }
}

// ntf_debug_log appends a line to the debug log. The file is opened and closed
// immediately so no handle is held on the user partition.
static void ntf_debug_log(QString const &line) {
    nh_log("%s", qPrintable(line));
    if (!ntf_debug)
        return;
    QFile f(NTF_DIR "/debug.log");
    if (f.size() > 64 * 1024)
        f.remove();
    if (!f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;
    QTextStream(&f) << QDateTime::currentDateTime().toString(Qt::ISODate) << " " << line << "\n";
    f.close();
}

static QString ntf_transform_str(QTransform const &t) {
    return QString("[%1 %2 %3 %4 dx=%5 dy=%6]")
        .arg(t.m11()).arg(t.m12()).arg(t.m21()).arg(t.m22()).arg(t.dx()).arg(t.dy());
}

extern "C" __attribute__((visibility("default"))) QTransform _ntf_transform_between(QScreen const *self, Qt::ScreenOrientation a, Qt::ScreenOrientation b, QRect const &target) {
    QTransform t = QScreen__transformBetween(self, a, b, target);
    bool flip = (b & ntf_flip_mask) != 0;

    if (flip) {
        // (x, y) -> (left+right-x, top+bottom-y), applied after the original transform.
        QTransform f(-1, 0, 0, -1, target.left() + target.right(), target.top() + target.bottom());
        t = t * f;
    }

    ntf_debug_log(QString("transformBetween a=%1 b=%2 target=%3,%4 %5x%6 flip=%7 result=%8")
        .arg(ntf_orientation_name(a)).arg(ntf_orientation_name(b))
        .arg(target.x()).arg(target.y()).arg(target.width()).arg(target.height())
        .arg(flip ? "yes" : "no").arg(ntf_transform_str(t)));

    return t;
}

static int ntf_init() {
    QSettings s(NTF_DIR "/settings.ini", QSettings::IniFormat);
    ntf_debug = s.value("TouchFix/Debug", false).toBool();

    if (s.contains("TouchFix/FlipOrientations")) {
        ntf_flip_mask = 0;
        for (QString const &name : s.value("TouchFix/FlipOrientations").toStringList()) {
            QString n = name.trimmed().toLower();
            if (n == "portrait")                ntf_flip_mask |= Qt::PortraitOrientation;
            else if (n == "landscape")          ntf_flip_mask |= Qt::LandscapeOrientation;
            else if (n == "inverted_portrait")  ntf_flip_mask |= Qt::InvertedPortraitOrientation;
            else if (n == "inverted_landscape") ntf_flip_mask |= Qt::InvertedLandscapeOrientation;
        }
    }

    ntf_debug_log(QString("init: hook=%1 flip_mask=0x%2")
        .arg(QScreen__transformBetween ? "ok" : "missing").arg(ntf_flip_mask, 0, 16));
    return 0;
}

static bool ntf_uninstall() {
    nh_delete_file(NTF_DIR "/settings.ini");
    nh_delete_file(NTF_DIR "/debug.log");
    nh_delete_dir(NTF_DIR);
    return true;
}

NickelHook(
    .init      = &ntf_init,
    .info      = &NickelTouchFix,
    .hook      = NickelTouchFixHook,
    .dlsym     = NickelTouchFixDlsym,
    .uninstall = &ntf_uninstall
)
