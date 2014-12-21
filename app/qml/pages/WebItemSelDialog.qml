import QtQuick 2.0
import QtQml 2.1
import Sailfish.Silica 1.0

Dialog {
    id: itemSelDialog
    property alias model: listView.model
    property int selectedIndex: -1

    SilicaListView {
        id: listView
        anchors.fill: parent

        VerticalScrollDecorator { flickable: webview }

        header: PageHeader {
        }

        delegate: ListItem {
            id: itemDelegate
            contentHeight: Theme.itemSizeSmall

            Label {
                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingMedium
                    right: parent.right
                    rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                text: model.text
                color: model.enabled ?
                           (itemDelegate.highlighted ? Theme.highlightColor : Theme.primaryColor)
                         : Theme.secondaryColor
                truncationMode: TruncationMode.Fade
            }

            enabled: model.enabled
            onClicked: {
                selectedIndex = model.index;
                accept();
            }
        }
    }
}
