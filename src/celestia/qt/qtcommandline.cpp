#include "qtcommandline.h"

#include <utility>

#include <QCommandLineParser>
#include <QDir>

#include <celutil/gettext.h>

namespace celestia::qt
{

CelestiaCommandLineOptions ParseCommandLine(const QCoreApplication& app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("3D visualization of space");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({
        { "dir", _("Set the data directory."), _("datadir") },
        { "conf", _("Set the configuraton file."), _("conf") },
        { "extrasdir", _("Add an extras directory. This option may be specified multiple times."), _("extrasdir") },
#ifdef CELX
        { "luahook", _("Add a trusted Lua hook file. This option may be specified multiple times."), _("file") },
#endif
        { "fullscreen", _("Start in fullscreen mode.") },
        { { "s", "nosplash" }, _("Skip the splash screen.") },
        { { "u", "url" }, _("Set the start cel:// URL or startup script path."), _("url") },
        { { "l", "log" }, _("Set the path to the log file."), _("logpath") },
    });

    parser.process(app);

    QDir currentDirectory;

    CelestiaCommandLineOptions options;
    options.startDirectory = parser.value("dir");

    // Resolve input paths before changing to the data directory.
    options.configFileName = parser.value("conf");
    if (!options.configFileName.isEmpty())
        options.configFileName = currentDirectory.absoluteFilePath(options.configFileName);

    auto absolutePaths = [&currentDirectory](QStringList paths)
    {
        for (auto& path : paths)
            path = currentDirectory.absoluteFilePath(path);
        return paths;
    };
    options.extrasDirectories = absolutePaths(parser.values("extrasdir"));
#ifdef CELX
    auto hooks = parser.values("luahook");
    hooks.removeAll(QString{});
    options.luaHooks = absolutePaths(std::move(hooks));
#endif

    // similarly if the startURL is not a cel: URL, it will be interpreted as
    // a path to a script, so convert that to absolute also
    options.startURL = parser.value("url");
    if (!options.startURL.isEmpty() && !options.startURL.startsWith("cel:"))
        options.startURL = currentDirectory.absoluteFilePath(options.startURL);

    // logFilename is processed before the directory change
    options.logFilename = parser.value("log");

    options.skipSplashScreen = parser.isSet("nosplash");
    options.startFullscreen = parser.isSet("fullscreen");

    return options;
}

} // end namespace celestia::qt
