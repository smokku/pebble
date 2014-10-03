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
            spacing: Theme.paddingLarge
            PageHeader {
                title: pebbled.name
            }

            Row {
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }


                Button {
                    text: "Ping"
                    width: parent.width / 2
                    onClicked: {
                        pebbled.ping(66)
                    }
                }

                Button {
                    text: "Sync Time"
                    width: parent.width / 2
                    onClicked: {
                        pebbled.time()
                    }
                }
            }

        }
    }
}
