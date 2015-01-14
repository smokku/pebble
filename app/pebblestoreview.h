#ifndef PEBBLESTOREVIEW_H
#define PEBBLESTOREVIEW_H

#include <private/qquickwebview_p.h>
#include <private/qwebnavigationrequest_p.h>
#include <QQuickItem>

class PebbleStoreView : public QQuickWebView
{
    Q_OBJECT
public:
    PebbleStoreView();

public slots:
    void onNavigationRequested(QWebNavigationRequest* request);

signals:
    void loginSuccess(const QString & accessToken);
    void downloadPebbleApp(const QString & title, const QString & downloadUrl);
    void call(const QString &, const QString &);
};

#endif // PEBBLESTOREVIEW_H
