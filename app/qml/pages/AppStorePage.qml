import QtQuick 2.0
import QtQml 2.1
import Sailfish.Silica 1.0
import org.pebbled 0.1
import org.nemomobile.configuration 1.0

Page {
    id: page

    ConfigurationGroup {
        id: settings
        path: "/org/pebbled/settings"
        property string storeAccessToken: ""
    }

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentHeight: column.height + webview.height

        PullDownMenu {
            visible: webview.loggedin;

            MenuItem {
                text: qsTr("Logout")
                onClicked: {
                    webview.logout();
                }
            }
        }

        Column {
            id: column
            width: page.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Pebble Appstore")
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: webview.loggedin;
                Button {
                    text: qsTr("WatchApps")
                    onClicked: {
                        webview.gotoWatchApps();
                    }
                }
                Button {
                    text: qsTr("WatchFaces")
                    onClicked: {
                        webview.gotoWatchFaces();
                    }
                }
            }

            Column {
                id: download
                visible: webview.downloadInProgress
                width: parent.width

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    id: downloadLabel
                    text: qsTr("Downloading...")
                }

                BusyIndicator {
                    anchors.horizontalCenter: parent.horizontalCenter
                    running: true
                    size: BusyIndicatorSize.Large
                }
            }
        }

        PebbleStoreView  {
            id: webview
            visible: !webview.downloadInProgress
            width: page.width
            height: page.height - column.height

            anchors {
                top: column.bottom
            }

            accessToken: settings.storeAccessToken

            onAccessTokenChanged: {
                settings.storeAccessToken = accessToken;
            }

            onDownloadPebbleApp: {
                downloadLabel.text = qsTr("Downloading %1...").arg(downloadTitle)
            }
        }
    }


}

