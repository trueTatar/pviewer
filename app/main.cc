#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <cstdlib>
#include <iostream>

#include "global_path.hpp"
#include "image_formats.hpp"
#include "main_window.hpp"

namespace {

QString EnsureDefaultDirectoryExists() {
  QDir dir(g_basicPath);
  if (dir.exists()) return g_basicPath;

  if (!QDir().mkpath(g_basicPath)) {
    std::cerr << "Failed to create default directory '" << g_basicPath.toStdString()
              << "'\nAborting..." << std::endl;
    std::exit(1);
  }

  return g_basicPath;
}

}  // namespace

MainWindow* CreateWindow(int argc, char* argv[]) {
  // Collect non-flag arguments; honour --no-autoshow by starting blank.
  QStringList args;
  bool no_autoshow = false;
  for (int i = 1; i < argc; ++i) {
    if (QString(argv[i]) == "--no-autoshow")
      no_autoshow = true;
    else
      args << argv[i];
  }

  if (no_autoshow) return new MainWindow();

  if (!args.isEmpty() && QFileInfo(args[0]).isDir()) {
    int start = args.size() >= 2 ? args[1].toInt() : 0;
    return new MainWindow(args[0], start);
  } else if (!args.isEmpty()) {
    QList<QString> images;
    for (const QString& filename : std::as_const(args)) {
      QFileInfo info(filename);
      if (!info.isFile()) {
        std::cout << '\'' << filename.toStdString() << '\''
                  << " is not a file\nAborting..." << std::endl;
        std::exit(1);
      }
      if (IsSupportedImageSuffix(info.suffix())) images.append(filename);
    }
    return new MainWindow(images);
  } else {
    return new MainWindow(EnsureDefaultDirectoryExists());
  }
}

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  MainWindow* ps = CreateWindow(argc, argv);
  ps->setWindowState(Qt::WindowMaximized | Qt::WindowFullScreen);
  ps->show();
  return app.exec();
}
