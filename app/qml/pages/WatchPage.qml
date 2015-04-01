import QtQuick 2.0
import QtQml 2.1
import Sailfish.Silica 1.0

Page {
    id: watchPage

    property bool firmwareVersionOK: false

    Component.onCompleted: {
        pebbled.info.firmware.forEach(function(firmware){
            if (!firmware.recovery) {
                firmwareVersionOK = (firmware.version.indexOf("v1.") !== 0)
            }
        })
    }

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentHeight: column.height

        VerticalScrollDecorator { flickable: flickable }

        Column {
            id: column
            width: watchPage.width

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
                    text: qsTr("Info")
                    width: parent.width / 3
                    onClicked: pageStack.push(Qt.resolvedUrl("WatchInfo.qml"))
                }

                Button {
                    text: qsTr("Ping")
                    width: parent.width / 3
                    onClicked: pebbled.ping(66)
                }

                Button {
                    text: qsTr("Sync Time")
                    width: parent.width / 3
                    onClicked: pebbled.time()
                }
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }

            Label {
                visible: !firmwareVersionOK

                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }
                font.pixelSize: Theme.fontSizeMedium
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.Center
                text: qsTr("Your firmware is too old to support SDKv2 applications")
                ColorAnimation on color {
                    from: Theme.primaryColor; to: Theme.highlightColor
                    duration: 2500
                    loops: Animation.Infinite
                    easing: { type: Easing.InOutQuint }
                }
            }

            Label {
                visible: firmwareVersionOK
                text: qsTr("Installed applications")
                font.family: Theme.fontFamilyHeading
                color: Theme.highlightColor
                anchors.right: parent.right
                anchors.rightMargin: Theme.paddingMedium
            }

            Repeater {
                visible: firmwareVersionOK
                id: slotsRepeater
                model: pebbled.appSlots

                ListItem {
                    id: slotDelegate
                    menu: slotMenu
                    contentHeight: Theme.itemSizeSmall

                    property bool isEmptySlot: modelData === ""
                    property var appInfo: pebbled.appInfoByUuid(modelData)
                    property bool isKnownApp: appInfo.hasOwnProperty("uuid")
                    property bool isLocalApp: appInfo.hasOwnProperty("isLocal") && appInfo.isLocal
                    property bool busy: false

                    function configure() {
                        var uuid = modelData;
                        pebbled.launchApp(uuid);
                        console.log("going to call configure on app with uuid " + uuid);
                        var url = pebbled.configureApp(uuid);
                        console.log("received url: " + url);
                        pageStack.push(Qt.resolvedUrl("AppConfigDialog.qml"), {
                                           url: url,
                                           uuid: uuid,
                                           name: appInfo.longName
                                       });
                    }

                    function remove() {
                        remorseAction(qsTr("Uninstalling"), function() {
                            busy = true;
                            pebbled.unloadApp(index);
                        });
                    }

                    function install() {
                        var dialog = pageStack.push(Qt.resolvedUrl("InstallAppDialog.qml"));
                        dialog.accepted.connect(function() {
                            var uuid = dialog.selectedUuid;

                            if (pebbled.isAppInstalled(uuid)) {
                                console.warn("uuid already installed");
                                return;
                            }

                            var slot = index;
                            console.log("installing " + uuid + " into " + slot);
                            busy = true;
                            pebbled.uploadApp(uuid, slot);
                        });

                    }

                    Item {
                        id: slotIcon
                        width: Theme.itemSizeSmall
                        height: Theme.itemSizeSmall

                        anchors {
                            top: parent.top
                            left: parent.left
                            leftMargin: Theme.paddingLarge
                        }

                        Image {
                            id: slotImage
                            anchors.centerIn: parent
                            source: isLocalApp ? "image://pebble-app-icon/" + modelData : ""
                            scale: 2
                            visible: !isEmptySlot && isLocalApp && !slotBusy.running
                        }

                        Rectangle {
                            width: 30
                            height: 30
                            anchors.centerIn: parent
                            scale: 2
                            border {
                                width: 2
                                color: slotDelegate.highlighted ? Theme.highlightColor : Theme.primaryColor
                            }
                            color: "transparent"
                            visible: isEmptySlot && !slotBusy.running
                        }

                        BusyIndicator {
                            id: slotBusy
                            anchors.centerIn: parent
                            running: slotDelegate.busy
                        }
                    }

                    Label {
                        id: slotName
                        anchors {
                            left: slotIcon.right
                            leftMargin: Theme.paddingMedium
                            right: parent.right
                            rightMargin: Theme.paddingLarge
                            verticalCenter: parent.verticalCenter
                        }
                        text: isEmptySlot ? qsTr("(empty slot)") : (isKnownApp ? (isLocalApp ? appInfo.longName : appInfo.shortName) : qsTr("(slot in use by unknown app)"))
                        color: slotDelegate.highlighted ? Theme.highlightColor : (isLocalApp ? Theme.primaryColor : Theme.secondaryColor )
                        onTextChanged: slotDelegate.busy = false;
                    }

                    Component {
                        id: slotMenu
                        ContextMenu {
                            MenuItem {
                                text: qsTr("Install app...")
                                visible: isEmptySlot
                                onClicked: install();
                            }
                            Label {
                                text: qsTr("Companion app missing")
                                color: Theme.secondaryColor
                                horizontalAlignment: Text.Center
                                font.pixelSize: Theme.fontSizeSmall
                                visible: !isEmptySlot && !isLocalApp
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                }
                                height: Theme.itemSizeSmall
                                verticalAlignment : Text.AlignVCenter
                            }
                            MenuItem {
                                text: qsTr("Configure...")
                                visible: !isEmptySlot && isLocalApp
                                onClicked: configure();
                            }
                            MenuItem {
                                text: qsTr("Uninstall")
                                visible: !isEmptySlot
                                onClicked: remove();
                            }
                        }
                    }

                    onClicked: {
                        if (isEmptySlot) {
                            install();
                        } else {
                            showMenu();
                        }
                    }
                }
            }
        }
    }
}
