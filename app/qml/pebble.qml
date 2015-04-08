import QtQuick 2.0
import Sailfish.Silica 1.0
import "pages"
import org.pebbled 0.1

ApplicationWindow
{
    id: app

    property string firmwareVersion
    property string recoveryVersion
    property string hardwareVersion
    property string firmwareLatest

    initialPage: Component { ManagerPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    function parseInfo() {
        if (pebbled.info.firmware && pebbled.info.firmware.length) {
            pebbled.info.firmware.forEach(function(firmware){
                if (firmware.recovery) {
                    recoveryVersion = firmware.version
                } else {
                    firmwareVersion = firmware.version
                    hardwareVersion = firmware.hardware
                }
            })
        } else {
            firmwareVersion = recoveryVersion = hardwareVersion = ""
        }
    }

    function notifyNewFirmware() {
        firmwareLatest = pebbleFirmware.latest.friendlyVersion || ""
        if (firmwareLatest && firmwareVersion) {
            pebbled.notifyFirmware(firmwareVersion === firmwareLatest);
        }
    }

    Component.onCompleted: {
        parseInfo()
    }

    Connections {
        target: pebbled
        onInfoChanged: {
            parseInfo()
        }
    }

    onHardwareVersionChanged: {
        if (hardwareVersion) {
            pebbleFirmware.updateLatest(hardwareVersion)
        }
    }

    onFirmwareVersionChanged: {
        notifyNewFirmware()
    }

    Connections {
        target: pebbleFirmware
        onLatestChanged: {
            notifyNewFirmware()
        }
    }
}
