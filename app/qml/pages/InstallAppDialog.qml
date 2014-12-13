import QtQuick 2.0
import QtQml 2.1
import Sailfish.Silica 1.0

Dialog {
    id: installAppPage

    property string selectedUuid;

    SilicaListView {
        id: appList
        anchors.fill: parent

        header: DialogHeader {
           title: qsTr("Install app")
           defaultAcceptText: qsTr("Install")
        }

        VerticalScrollDecorator { flickable: flickable }

        currentIndex: -1

        delegate: ListItem {
            id: appDelegate
            contentHeight: Theme.itemSizeSmall

            property string uuid: modelData.uuid
            property bool alreadyInstalled: pebbled.isAppInstalled(uuid)

            Image {
                id: appImage
                anchors {
                    top: parent.top
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                }
                width: Theme.itemSizeSmall
            }

            Label {
                id: appName
                anchors {
                    left: appImage.right
                    leftMargin: Theme.paddingMedium
                    right: parent.right
                    rightMargin: Theme.paddiumLarge
                    verticalCenter: parent.verticalCenter
                }
                text: modelData.longName
                color: appDelegate.highlighted ? Theme.highlightColor : Theme.primaryColor
            }

            onClicked: {
                appList.currentIndex = index
                if (!alreadyInstalled) {
                    selectedUuid = uuid
                    accept();
                }
            }
        }

        model: pebbled.allApps
    }

    canAccept: appList.currentIndex >= 0 && !appList.currentItem.alreadyInstalled
}
