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
import watch 0.1
import org.nemomobile.dbus 1.0

Page {
    id: page

    property string name
    property string address
    property bool connected: false

    onNameChanged: console.log(name)
    onAddressChanged: console.log(address)
    onConnectedChanged: console.log(connected?"connected":"disconnected")

    WatchConnector {
        id: watchConnector
    }

    DBusInterface {
        id: pebbled
        destination: "org.pebbled"
        path: "/"
        iface: "org.pebbled"
        signalsEnabled: true

        function pebbleChanged() {
            page.name = getProperty("name");
            page.address = getProperty("address");
        }
        function connectedChanged() {
            page.connected = getProperty("connected");
        }

        Component.onCompleted: {
            pebbled.pebbleChanged();
            pebbled.connectedChanged();
        }
    }

    SilicaFlickable {
        anchors.fill: parent

        contentHeight: column.height

        Column {
            id: column

            width: page.width
            spacing: Theme.paddingLarge
            PageHeader {
                title: "Pebble Manager"
            }
            Label {
                visible: !page.connected
                text: "Waiting for watch...\nIf it can't be found plase\ncheck it's available and\npaired in Bluetooth settings."
                width: column.width
            }
            ListItem {
                visible: !!page.name
                Label {
                    text: page.name
                }
                onVisibleChanged: {
                    if (parent.visible) {
                        // Connect with the device
                        watchConnector.deviceConnect(page.name, page.address);
                    }
                }
            }
            Button {
                text: "Ping"
                onClicked: {
                    watchConnector.ping(66)
                }
            }
            Button {
                text: "Send SMS"
                onClicked: {
                    watchConnector.sendSMSNotification("Dummy", "Hello world!")
                }
            }
            Button {
                text: "Ring"
                onClicked: {
                    watchConnector.ring("+1234567890", "Test user")
                }
            }
            Button {
                text: "Start call"
                onClicked: {
                    watchConnector.startPhoneCall()
                }
            }
            Button {
                text: "End call"
                onClicked: {
                    watchConnector.endPhoneCall()
                }
            }
        }
    }
}
