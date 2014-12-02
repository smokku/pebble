import QtQuick 2.0
import QtQml 2.1
import QtWebKit 3.0
import Sailfish.Silica 1.0

Page {
    id: appConfigPage

    property alias url: webview.url
    property string uuid

    SilicaWebView {
        id: webview
        anchors.fill: parent

        header: PageHeader {
            title: "Configuring " + uuid
        }

        onNavigationRequested: {
            console.log("appconfig navigation requested to " + request.url);
            var url = request.url.toString();
            if (/^pebblejs:\/\/close/.exec(url)) {
                var data = decodeURI(url.substring(17));
                console.log("appconfig requesting close; data: " + data);
                pebbled.setAppConfiguration(uuid, data);
                pageStack.pop();
                request.action = WebView.IgnoreRequest;
            } else {
                request.action = WebView.AcceptRequest;
            }
        }
    }
}
