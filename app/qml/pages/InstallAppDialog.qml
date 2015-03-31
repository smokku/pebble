import QtQuick 2.0
import QtQml 2.1
import Sailfish.Silica 1.0
import QtDocGallery 5.0
import Sailfish.Pickers 1.0

Dialog {
    id: installAppPage

    property string selectedUuid;

    Component {
        id: appPicker

        PickerDialog {
            id: appPickerDialog
            title: qsTr("Select App files")

            SilicaListView {
                id: listView

                currentIndex: -1
                anchors.fill: parent

                model: documentModel.model

                DocumentModel {
                    id: documentModel
                    selectedModel: _selectedModel
                    contentFilter: GalleryStartsWithFilter {
                        property: "filePath"
                        value: StandardPaths.documents + "/../Downloads"
                    }
                }

                header: PageHeader {
                    id: pageHeader
                    title: appPickerDialog.title
                }


                delegate: DocumentItem {
                    id: documentItem
                    baseName: Theme.highlightText(documentModel.baseName(model.fileName), documentModel.filter, Theme.highlightColor)
                    extension: Theme.highlightText(documentModel.extension(model.fileName), documentModel.filter, Theme.highlightColor)
                    selected: model.selected

                    ListView.onAdd: AddAnimation { target: documentItem; duration: _animationDuration }
                    ListView.onRemove: RemoveAnimation { target: documentItem; duration: _animationDuration }
                    onClicked: documentModel.updateSelected(index, !selected)
                }

                VerticalScrollDecorator {}
            }
        }

    }

    SilicaListView {
        id: appList
        anchors.fill: parent

        header: DialogHeader {
           title: qsTr("Install App")
           defaultAcceptText: qsTr("Install")
        }

        VerticalScrollDecorator { flickable: flickable }

        PullDownMenu {
            MenuItem {
                text: qsTr("Add App file")
                onClicked: {
                    var addApps = function() {
                        for(var i=0; i < picker.selectedContent.count; ++i) {
                            var appPath = picker.selectedContent.get(i).filePath
                            console.log(appPath)
                            pebbled.registerAppFile(appPath)
                        }
                        picker.selectedContentChanged.disconnect(addApps)
                    }
                    var picker = pageStack.push(appPicker)
                    picker.selectedContentChanged.connect(addApps)
                }

            }
        }

        currentIndex: -1

        delegate: ListItem {
            id: appDelegate
            contentHeight: modelData.isLocal ? Theme.itemSizeSmall : 0

            visible: modelData.isLocal

            property string uuid: modelData.uuid
            property bool alreadyInstalled: pebbled.isAppInstalled(uuid)

            Item {
                id: appIcon
                width: Theme.itemSizeSmall
                height: Theme.itemSizeSmall

                anchors {
                    top: parent.top
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                }

                Image {
                    id: appImage
                    anchors.centerIn: parent
                    source: appDelegate.visible ? "image://pebble-app-icon/" + uuid : ""
                    scale: 2
                }
            }

            Label {
                id: appName
                anchors {
                    left: appIcon.right
                    leftMargin: Theme.paddingMedium
                    right: parent.right
                    rightMargin: Theme.paddingLarge
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
