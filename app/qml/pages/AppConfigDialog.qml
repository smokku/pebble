import QtQuick 2.0
import QtQml 2.1
import QtWebKit 3.0
import Sailfish.Silica 1.0

Dialog {
    id: appConfigPage

    property alias url: webview.url
    property string uuid
    property string name

    SilicaWebView {
        id: webview
        visible: url != ""
        anchors.fill: parent

        header: DialogHeader {
            title: "Configuring " + name
        }

        VerticalScrollDecorator { flickable: webview }

        overridePageStackNavigation: true

        onNavigationRequested: {
            console.log("appconfig navigation requested to " + request.url);
            var url = request.url.toString();
            if (/^pebblejs:\/\/close/.exec(url)) {
                var data = decodeURIComponent(url.substring(17));
                console.log("appconfig requesting close; data: " + data);
                pebbled.setAppConfiguration(uuid, data);
                appConfigPage.canAccept = true;
                appConfigPage.accept();
                request.action = WebView.IgnoreRequest;
            } else {
                request.action = WebView.AcceptRequest;
            }
        }

        experimental.itemSelector: Component {
            Item {
                Component.onCompleted: {
                    var dialog = pageStack.push(Qt.resolvedUrl("WebItemSelDialog.qml"), {
                                                       model: model.items
                                                });
                    dialog.onRejected.connect(function() {
                        model.reject();
                    });
                    dialog.onAccepted.connect(function() {
                        model.accept(dialog.selectedIndex);
                    });
                }
            }
        }
    }

    ProgressBar {
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        visible: webview.visible && webview.loading
        minimumValue: 0
        maximumValue: 100
        indeterminate: webview.loadProgress === 0
        value: webview.loadProgress
    }

    Text {
        anchors.centerIn: parent
        visible: url == ""
        text: qsTr("No configuration settings available")
        width: parent.width - 2*Theme.paddingLarge
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        font {
            pixelSize: Theme.fontSizeLarge
            family: Theme.fontFamilyHeading
        }
        color: Theme.highlightColor
    }

    canAccept: false
}
