#ifndef PEBBLEFIRMWARE_H
#define PEBBLEFIRMWARE_H

#include <QObject>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class PebbleFirmware : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QJsonObject latest READ latest NOTIFY latestChanged)

    QJsonObject latest() { return _latest; }

public:
    explicit PebbleFirmware(QObject *parent = 0);

    const static QString firmwareURL;

signals:
    void latestChanged();
    void firmwareFetched(QString pbz);

public slots:
    void updateLatest(QString hw);
    void fetchFirmware(QString type);

private:
    QJsonObject _latest;

    QNetworkAccessManager *nm;

private slots:
    void onNetworkReplyFinished(QNetworkReply*);
};

#endif // PEBBLEFIRMWARE_H
