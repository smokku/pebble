#include "bundle.h"
#include <QSharedData>
#include <QScopedPointer>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <quazip/quazipfile.h>

class BundleData : public QSharedData {
public:
    bool isValid;
    QString path;
    QJsonObject manifest;

    BundleData() : isValid(false) {}
};

QLoggingCategory Bundle::l("Bundle");

Bundle::Bundle() : b(new BundleData)
{}

Bundle::Bundle(const Bundle &rhs) : b(rhs.b)
{}

Bundle &Bundle::operator=(const Bundle &rhs)
{
    if (this != &rhs)
        b.operator=(rhs.b);
    return *this;
}

Bundle::~Bundle()
{}

QString Bundle::type() const
{
    return b->manifest.value("type").toString();
}

QString Bundle::path() const
{
    return b->path;
}

bool Bundle::isValid() const
{
    return b->isValid;
}

Bundle Bundle::fromPath(const QString &path)
{
    Bundle bundle;

    QFileInfo bundlePath(path);
    if (!bundlePath.isReadable()) {
        qCWarning(l) << "bundle" << bundlePath.absolutePath() << "is not readable";
        return Bundle();
    }

    bundle.b->path = path;

    QScopedPointer<QIODevice> manifestJSON(bundle.openFile(Bundle::MANIFEST, QIODevice::Text));
    if (!manifestJSON) {
        qCWarning(l) << "cannot find" << path << "manifest json";
        return Bundle();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(manifestJSON->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(l) << "cannot parse" << path << "manifest json" << parseError.errorString();
        return Bundle();
    }
    manifestJSON->close();
    bundle.b->manifest = doc.object();

    bundle.b->isValid = true;
    return bundle;
}

QIODevice *Bundle::openFile(enum Bundle::File file, QIODevice::OpenMode mode) const
{
    QString fileName;
    switch (file) {
    case Bundle::MANIFEST:
        fileName = "manifest.json";
        break;
    case Bundle::INFO:
        fileName = "appinfo.json";
        break;
    case Bundle::APPJS:
        fileName = "pebble-js-app.js";
        break;
    case Bundle::BINARY:
        fileName = b->manifest.value(type()).toObject().value("name").toString();
        break;
    case Bundle::RESOURCES:
        fileName = b->manifest.value("resources").toObject().value("name").toString();
        break;
    }

    QIODevice *dev = 0;
    QFileInfo bundlePath(path());
    if (bundlePath.isDir()) {
        QDir bundleDir(path());
        if (bundleDir.exists(fileName)) {
            dev = new QFile(bundleDir.absoluteFilePath(fileName));
        }
    } else if (bundlePath.isFile()) {
        dev =  new QuaZipFile(path(), fileName);
    }

    if (dev && !dev->open(QIODevice::ReadOnly | mode)) {
        delete dev;
        return 0;
    }

    return dev;
}

bool Bundle::fileExists(enum Bundle::File file) const
{
    QIODevice *dev = openFile(file);
    bool exists = dev && dev->isOpen();
    delete dev;
    return exists;
}

quint32 Bundle::crcFile(enum Bundle::File file) const
{
    quint32 ret = 0;

    switch (file) {
    case Bundle::BINARY:
        ret = b->manifest.value(type()).toObject().value("crc").toDouble();
        break;
    case Bundle::RESOURCES:
        ret = b->manifest.value("resources").toObject().value("crc").toDouble();
        break;
    default:
        qCWarning(l) << "Unsupported CRC for" << file;
    }

    return ret;
}
