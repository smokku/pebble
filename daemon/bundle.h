#ifndef BUNDLE_H
#define BUNDLE_H

#include <QLoggingCategory>
#include <QSharedDataPointer>
#include <QString>
#include <QIODevice>

class BundleData;

class Bundle
{
    Q_GADGET

    static QLoggingCategory l;

public:
    enum File {
        MANIFEST,
        INFO,
        BINARY,
        RESOURCES,
        APPJS
    };

    static Bundle fromPath(const QString &path);

    Bundle();
    Bundle(const Bundle &);
    Bundle &operator=(const Bundle &);
    ~Bundle();

    QString type() const;
    QString path() const;
    bool isValid() const;

    QIODevice *openFile(enum File, QIODevice::OpenMode = 0) const;
    bool fileExists(enum File) const;
    quint32 crcFile(enum File) const;

private:
    QSharedDataPointer<BundleData> b;
};

#endif // BUNDLE_H
