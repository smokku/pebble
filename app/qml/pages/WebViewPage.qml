import QtQuick 2.0
import QtQml 2.1
import QtWebKit 3.0
import Sailfish.Silica 1.0

Page {
    id: webviewPage

    property alias url: webview.url

    SilicaWebView {
        id: webview
        anchors.fill: parent

        onNavigationRequested: {
            console.log("navigation requested to " + request.url);
            var url = request.url.toString()
            if (/^pebblejs:\/\/close/.exec(url)) {
                var data = decodeURI(url.substring(17));
                console.log("match with pebble close regexp. data: " + data);
                pebbled.webviewClosed(data);
                pageStack.pop();
                request.action = WebView.IgnoreRequest;
            } else {
                request.action = WebView.AcceptRequest;
            }
        }
    }
}
