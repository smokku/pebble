import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentHeight: column.height

        VerticalScrollDecorator { flickable: flickable }

        Column {
            id: column
            width: page.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: "pebbled"
            }
            Label {
                wrapMode: Text.Wrap
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingSmall
                }
                font.pixelSize: Theme.fontSizeMedium
                horizontalAlignment: Text.AlignJustify
                text: qsTr(
                          "Unfortunately Harbour (Jolla Shop) does not allow running background services, "+
                          "thus background service option is not included in this version.")
            }
            Label {
                wrapMode: Text.Wrap
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingSmall
                }
                font.pixelSize: Theme.fontSizeMedium
                horizontalAlignment: Text.AlignJustify
                text: qsTr(
                          "You may be interested in installing Pebble application from OpenRepos, "+
                          "which does not include such restriction.")
            }
            Button {
                text: "Go to OpenRepos"
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }
                onClicked: Qt.openUrlExternally("http://openrepos.net/content/smoku/pebble")
            }
        }
    }
}
