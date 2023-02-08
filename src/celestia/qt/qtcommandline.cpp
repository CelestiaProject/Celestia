#include "qtcommandline.h"

#include <QCommandLineParser>
#include <QDir>

#include <celutil/gettext.h>

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
        { "fullscreen", _("Start in fullscreen mode.") },
        { { "s", "nosplash" }, _("Skip the splash screen.") },
        { { "u", "url" }, _("Set the start cel:// URL or startup script path."), _("url") },
        { { "l", "log" }, _("Set the path to the log file."), _("logpath") },
    });

    parser.process(app);

    QDir currentDirectory;

    CelestiaCommandLineOptions options;
    options.startDirectory = parser.value("dir");

    // configFileName and extrasDirectories are handled AFTER changing the
    // current directory to the startDirectory, so convert them to absolute
    // paths here.
    options.configFileName = currentDirectory.absoluteFilePath(parser.value("conf"));
    options.extrasDirectories = parser.values("extrasdir");
    for (auto& extrasDir : options.extrasDirectories)
    {
        extrasDir = currentDirectory.absoluteFilePath(extrasDir);
    }

    // similarly if the startURL is not a cel: URL, it will be interpreted as
    // a path to a script, so convert that to absolute also
    options.startURL = parser.value("url");
    if (!options.startURL.startsWith("cel:"))
    {
        options.startURL = currentDirectory.absoluteFilePath(options.startURL);
    }

    // logFilename is processed before the directory change
    options.logFilename = parser.value("log");

    options.skipSplashScreen = parser.isSet("nosplash");
    options.startFullscreen = parser.isSet("fullscreen");

    return options;
}
