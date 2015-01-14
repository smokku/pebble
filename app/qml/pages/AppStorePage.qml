import QtQuick 2.0
import QtQml 2.1
import Sailfish.Silica 1.0
import org.pebbled 0.1

Page {
    id: page

    PebbleStoreView  {
        id: webview
        anchors.fill: parent
        url: "https://auth.getpebble.com/oauth/authorize?client_id=f88739e8e7a696c411236c41afc81cbef16dc54c3ff633d92dd4ceb0e5a25e5f&response_type=token&mid=xxx&pid=xxx&platform=android&mobile=sign_in&redirect_uri=pebble%3A%2F%2Flogin"

        onLoginSuccess: {
            console.log("ON Login " + accessToken);
            webview.url = "https://apps-prod.getpebble.com/en_US/?access_token=" + accessToken + "#/watchfaces"
        }

        onDownloadPebbleApp: {
            console.log("ON DOWNLOAD " + title);
            console.log(downloadUrl);
        }
    }
}
