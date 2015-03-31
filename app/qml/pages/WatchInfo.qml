import QtQuick 2.0
import QtQml 2.1
import Sailfish.Silica 1.0

Page {
    id: watchInfoPage

    property string firmwareVersion
    property string recoveryVersion

    Component.onCompleted: {
        pebbled.info.firmware.forEach(function(firmware){
            if (firmware.recovery) {
                recoveryVersion = firmware.version
            } else {
                firmwareVersion = firmware.version
            }
        })
    }

    Column {
        id: column
        width: watchInfoPage.width
        spacing: Theme.paddingMedium

        PageHeader {
            title: pebbled.name
        }

        Grid {
            columns: 2
            spacing: Theme.paddingMedium
            anchors {
                left: parent.left
                right: parent.right
                margins: Theme.paddingLarge
            }

            Label {
                color: Theme.highlightColor
                text: qsTr("Address")
            }
            Label {
                text: pebbled.info.address
            }

            Label {
                color: Theme.highlightColor
                text: qsTr("Serial Number")
            }
            Label {
                text: pebbled.info.serial
            }

            Label {
                color: Theme.highlightColor
                text: qsTr("BootLoader")
            }
            Label {
                text: new Date(pebbled.info.bootloader * 1000).toLocaleString(Qt.locale(), Locale.ShortFormat)
            }

            Label {
                color: Theme.highlightColor
                text: qsTr("Firmware")
            }
            Label {
                text: firmwareVersion
            }

            Label {
                color: Theme.highlightColor
                text: qsTr("Recovery")
            }
            Label {
                text: recoveryVersion
            }
        }
    }
}
