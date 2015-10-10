import QtQuick 2.0
import QtQml 2.1
import Sailfish.Silica 1.0
import org.pebbled 0.1
import org.nemomobile.configuration 1.0

Page {
    id: page

    property bool showSearch;

    ConfigurationGroup {
        id: settings
        path: "/org/pebbled/settings"
        property string storeAccessToken: ""
    }

    SilicaFlickable {
        id: flickable
        anchors.top: parent.top
        width: parent.width
        height: column.height

        PullDownMenu {
            visible: webview.loggedin;

            MenuItem {
                text: qsTr("Logout")
                onClicked: {
                    remorse.execute(qsTr("Logging out..."), function() {
                        webview.logout();
                    });
                }
            }

            MenuItem {
                text: showSearch ? qsTr("Hide search") : qsTr("Show search");
                onClicked: {
                    showSearch = !showSearch;
                }
            }
        }

        Column {
            id: column
            width: page.width

            PageHeader {
                id: pageHeadTitle
                title: qsTr("Pebble Appstore")
            }

            SearchField {
                width: parent.width
                visible: webview.loggedin && showSearch
                id: searchField
                onTextChanged: {
                    var q = searchField.text.trim();
                    if (q.length >= 2) {
                        webview.searchQuery(q);
                    }
                }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: webview.loggedin

                IconButton {
                    id: backButton
                    enabled: webview.canGoBack
                    icon.source: "image://theme/icon-m-back"
                    onClicked: {
                        webview.goBack();
                    }
                }

                Button {
                    text: qsTr("Apps")
                    width: (page.width - loadingIndicator.width - backButton.width - Theme.paddingMedium) / 2
                    onClicked: {
                        webview.gotoWatchApps();
                    }
                }

                Button {
                    text: qsTr("Faces")
                    width: (page.width - loadingIndicator.width - backButton.width - Theme.paddingMedium) / 2
                    onClicked: {
                        webview.gotoWatchFaces();
                    }
                }

                BusyIndicator {
                    id: loadingIndicator
                    running: webview.loading
                    size: BusyIndicatorSize.Medium
                }
            }

            Column {
                id: download
                visible: webview.downloadInProgress
                width: parent.width
                spacing: Theme.paddingLarge

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
    }

        RemorsePopup {
            id: remorse
        }

        PebbleStoreView  {
            id: webview
            visible: !webview.downloadInProgress
            width: page.width
            height: page.height - column.height

            anchors {
                top: flickable.bottom
            }

            accessToken: settings.storeAccessToken
            hardwarePlatform: pebbled.info.platform

            onAccessTokenChanged: {
                settings.storeAccessToken = accessToken;
            }

            onDownloadPebbleApp: {
                downloadLabel.text = qsTr("Downloading %1...").arg(downloadTitle)
            }

            onTitleChanged: {
                pageHeadTitle.title = title;
            }
        }


}

