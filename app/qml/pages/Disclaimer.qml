import QtQuick 2.0
import QtQml 2.1
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
                title: qsTr("Feature unavailable")
            }
            Label {
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
                text: qsTr("This feature is available for supporters only.")
            }
            Button {
                text: qsTr("PayPal Donate")
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge * 2
                }
                onClicked: Qt.openUrlExternally(donate.url)
            }

            Label {
                text: qsTr("Supporter?")
                font.family: Theme.fontFamilyHeading
                color: Theme.highlightColor
                anchors.right: parent.right
                anchors.rightMargin: Theme.paddingMedium
            }
            Button {
                text: qsTr("Send me my code!")
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge * 2
                }
                onClicked: Qt.openUrlExternally("mailto:support@xiaoka.com?subject=pebbled code request - "+
                                                donate.id + "&body=My paypal id is: ")
            }
            Label {
                text: qsTr("Activation code")
                font.family: Theme.fontFamilyHeading
                color: Theme.highlightColor
                anchors.left: parent.left
                anchors.leftMargin: Theme.paddingMedium
            }
            TextField {
                id: code
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingMedium
                }
                focus: true
            }
        }
    }
}
