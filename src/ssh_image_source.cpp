#include "ssh_image_source.hpp"

#include <QBuffer>
#include <QImageReader>
#include <QProcess>
#include <QUrl>

#include "image_formats.hpp"
#include "image_list_order.hpp"

namespace {

constexpr int kListTimeoutMs = 30000;
constexpr int kLoadTimeoutMs = 120000;

struct SshLocation {
  QString user_name;
  QString host;
  int port = -1;
  QString path;
};

QString ShellQuote(QString value) {
  value.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
  return QStringLiteral("'") + value + QStringLiteral("'");
}

QStringList SshBaseArguments(SshLocation const& location) {
  QStringList args{
      QStringLiteral("-T"),
      QStringLiteral("-o"),
      QStringLiteral("BatchMode=yes"),
      QStringLiteral("-o"),
      QStringLiteral("ConnectTimeout=10"),
  };
  if (location.port > 0) {
    args << QStringLiteral("-p") << QString::number(location.port);
  }

  QString target = location.host;
  if (!location.user_name.isEmpty()) {
    target = location.user_name + QStringLiteral("@") + target;
  }
  args << target;
  return args;
}

bool ParseSshUrl(QString const& value, SshLocation* location, QString* error) {
  QUrl url(value);
  if (!url.isValid() || url.host().isEmpty()) {
    if (error != nullptr) {
      *error = QStringLiteral("Invalid SSH/SFTP URL: ") + value;
    }
    return false;
  }

  const QString scheme = url.scheme().toLower();
  if (scheme != QStringLiteral("sftp") && scheme != QStringLiteral("ssh")) {
    if (error != nullptr) {
      *error = QStringLiteral("Expected sftp:// or ssh:// URL: ") + value;
    }
    return false;
  }

  const QString path = url.path(QUrl::FullyDecoded);
  if (path.isEmpty()) {
    if (error != nullptr) {
      *error = QStringLiteral("Remote path is empty: ") + value;
    }
    return false;
  }

  *location = SshLocation{url.userName(QUrl::FullyDecoded), url.host(),
                          url.port(-1), path};
  return true;
}

QString RemoteFindCommandPrefix(QString const& path) {
  QStringList suffix_expressions;
  for (QString const& suffix : SupportedImageSuffixes()) {
    suffix_expressions << QStringLiteral("-iname ") +
                              ShellQuote(QStringLiteral("*.") + suffix);
  }

  return QStringLiteral("find ") + ShellQuote(path) +
         QStringLiteral(" -maxdepth 1 -type f \\( ") +
         suffix_expressions.join(QStringLiteral(" -o ")) +
         QStringLiteral(" \\)");
}

QString RemoteFindCommand(QString const& path) {
  return RemoteFindCommandPrefix(path) +
         QStringLiteral(" -printf '%T@\\0%p\\0'");
}

QString RemoteFindPathsCommand(QString const& path) {
  return RemoteFindCommandPrefix(path) + QStringLiteral(" -print0");
}

QString RemoteCatCommand(QString const& path) {
  return QStringLiteral("cat -- ") + ShellQuote(path);
}

QString UrlForRemotePath(SshLocation const& base, QString const& path) {
  QUrl url;
  url.setScheme(QStringLiteral("sftp"));
  url.setUserName(base.user_name);
  url.setHost(base.host);
  if (base.port > 0) url.setPort(base.port);
  url.setPath(path);
  return url.toString(QUrl::FullyEncoded);
}

QList<ImageListEntry> ParseRemoteImageEntries(QByteArray const& bytes) {
  const QList<QByteArray> fields = bytes.split('\0');
  QList<ImageListEntry> entries;
  entries.reserve(fields.size() / 2);
  for (int i = 0; i + 1 < fields.size(); i += 2) {
    bool ok = false;
    const double modified_seconds = fields[i].toDouble(&ok);
    if (!ok || fields[i + 1].isEmpty()) continue;
    entries.push_back({QString::fromUtf8(fields[i + 1]),
                       static_cast<qint64>(modified_seconds * 1000.0)});
  }
  return entries;
}

QList<ImageListEntry> ParseRemoteImagePaths(QByteArray const& bytes) {
  QList<ImageListEntry> entries;
  for (QByteArray const& field : bytes.split('\0')) {
    if (!field.isEmpty()) entries.push_back({QString::fromUtf8(field), 0});
  }
  return entries;
}

bool RunSshCommand(SshLocation const& location, QString const& remote_command,
                   int timeout_ms, QByteArray* stdout_data, QString* error) {
  QProcess process;
  QStringList args = SshBaseArguments(location);
  args << remote_command;
  process.start(QStringLiteral("ssh"), args);

  if (!process.waitForStarted()) {
    if (error != nullptr) {
      *error = QStringLiteral("Failed to start ssh: ") + process.errorString();
    }
    return false;
  }

  if (!process.waitForFinished(timeout_ms)) {
    process.kill();
    process.waitForFinished();
    if (error != nullptr) {
      *error = QStringLiteral("SSH command timed out");
    }
    return false;
  }

  if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
    if (error != nullptr) {
      QString stderr_text =
          QString::fromUtf8(process.readAllStandardError()).trimmed();
      *error = stderr_text.isEmpty()
                   ? QStringLiteral("SSH command failed")
                   : QStringLiteral("SSH command failed: ") + stderr_text;
    }
    return false;
  }

  if (stdout_data != nullptr) *stdout_data = process.readAllStandardOutput();
  return true;
}

}  // namespace

bool IsSshImageUrl(QString const& location) {
  QUrl url(location);
  const QString scheme = url.scheme().toLower();
  return (scheme == QStringLiteral("sftp") ||
          scheme == QStringLiteral("ssh")) &&
         url.isValid() && !url.host().isEmpty();
}

QList<QString> ListSshImages(QString const& directory_url, QString* error) {
  SshLocation location;
  if (!ParseSshUrl(directory_url, &location, error)) return {};

  QByteArray output;
  bool has_modification_times = true;
  if (!RunSshCommand(location, RemoteFindCommand(location.path), kListTimeoutMs,
                     &output, error)) {
    // BSD find has no -printf. Keep SSH browsing functional there, with the
    // natural-name ordering used as the deterministic fallback.
    if (!RunSshCommand(location, RemoteFindPathsCommand(location.path),
                       kListTimeoutMs, &output, error)) {
      return {};
    }
    if (error != nullptr) error->clear();
    has_modification_times = false;
  }

  QList<ImageListEntry> entries = has_modification_times
                                      ? ParseRemoteImageEntries(output)
                                      : ParseRemoteImagePaths(output);
  SortImagesNewestFirst(entries);

  QList<QString> urls;
  urls.reserve(entries.size());
  for (ImageListEntry const& entry : entries) {
    urls << UrlForRemotePath(location, entry.path);
  }
  return urls;
}

QImage LoadSshImage(QString const& image_url, QString* error) {
  SshLocation location;
  if (!ParseSshUrl(image_url, &location, error)) return {};

  QByteArray output;
  if (!RunSshCommand(location, RemoteCatCommand(location.path), kLoadTimeoutMs,
                     &output, error)) {
    return {};
  }

  QBuffer buffer(&output);
  QImageReader reader(&buffer);
  reader.setAutoTransform(true);
  QImage image = reader.read();
  if (image.isNull()) {
    if (error != nullptr) {
      *error = QStringLiteral("Failed to decode remote image: ") + image_url;
    }
    return {};
  }
  return image;
}
