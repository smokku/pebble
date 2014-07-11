/*
  Copyright (C) 2014 Jouni Roivas
  Copyright (C) 2013 Jolla Ltd.
  Contact: Thomas Perl <thomas.perl@jollamobile.com>
  All rights reserved.

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the authors nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

import QtQuick 2.0
import QtQml 2.1
import Sailfish.Silica 1.0

Page {
    id: page

    SilicaFlickable {
        anchors.fill: parent

        contentHeight: column.height

        Column {
            id: column

            width: page.width
            spacing: Theme.paddingLarge
            PageHeader {
                title: qsTr("Pebble Manager")
            }

            Label {
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                visible: pebbled.active && !pebbled.connected
                text: qsTr("Waiting for watch...\nIf it can't be found plase check it's available and paired in Bluetooth settings.")
                wrapMode: Text.Wrap
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }

            }
            ListItem {
                visible: pebbled.connected
                Label {
                    text: pebbled.name
                    truncationMode: TruncationMode.Fade
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: Theme.paddingLarge
                    }
                }
                onClicked: pageStack.push(Qt.resolvedUrl("WatchPage.qml"))
            }

            Label {
                text: qsTr("Service")
                font.family: Theme.fontFamilyHeading
                color: Theme.highlightColor
                anchors.right: parent.right
                anchors.rightMargin: Theme.paddingMedium
            }
            TextSwitch {
                text: qsTr("Enabled")
                description: pebbled.enabled ? qsTr("Automatic startup") : qsTr("Manual startup")
                checked: pebbled.enabled
                automaticCheck: false
                onClicked: pebbled.setEnabled(!checked)
            }
            TextSwitch {
                text: qsTr("Active")
                description: pebbled.active ? qsTr("Running") : qsTr("Dead")
                checked: pebbled.active
                automaticCheck: false
                onClicked: pebbled.setActive(!checked)
            }
            TextSwitch {
                text: qsTr("Connection")
                description: pebbled.connected ? qsTr("Connected"): qsTr("Disconnected")
                checked: pebbled.connected
                automaticCheck: false
                onClicked: {
                    if (pebbled.connected) {
                        pebbled.disconnect();
                    } else {
                        pebbled.reconnect();
                    }
                }
            }

            Label {
                text: qsTr("Settings")
                font.family: Theme.fontFamilyHeading
                color: Theme.highlightColor
                anchors.right: parent.right
                anchors.rightMargin: Theme.paddingMedium
            }
            TextSwitch {
                text: qsTr("Silent when connected")
                checked: false
                automaticCheck: false
                onClicked: {
                    console.log('settings.silentConnected');
                }
            }
        }
    }
}
