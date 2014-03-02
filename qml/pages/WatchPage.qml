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
import QtBluetooth 5.0
import Sailfish.Bluetooth 1.0
import Bluetooth 0.0
import org.nemomobile.voicecall 1.0
import org.nemomobile.notifications 1.0
import org.nemomobile.messages.internal 1.0
import org.nemomobile.commhistory 1.0
import watch 0.1

Page {
    id: page
    property alias watchConnector: watchConnector
    WatchConnector {
        id: watchConnector

        onHangup: {
            // Watch instantiated hangup, follow the orders
            manager.hangupAll();
        }
    }
    property var callerDetails: new Object

    // This actually handles the voice call in SailfisOS
    VoiceCallManager {
        id:manager

        function hangupAll() {
            for (var index = 0; index < voiceCalls.count; index++) {
                 voiceCalls.instance(index).hangup();
            }
        }

        onError: {
            console.log("Error: "+message)
            watchConnector.endPhoneCall();
        }

        function updateState() {
            //This needs cleaning up...
            var statusPriority = [
                        VoiceCall.STATUS_ALERTING,
                        VoiceCall.STATUS_DIALING,
                        VoiceCall.STATUS_DISCONNECTED,
                        VoiceCall.STATUS_ACTIVE,
                        VoiceCall.STATUS_HELD,
                        VoiceCall.STATUS_NULL
                    ]

            var newPrimaryCall = null
            for (var p = 0; p < statusPriority.length; p++) {
                for (var i = 0; i < voiceCalls.count; i++) {
                    if (voiceCalls.instance(i).status === statusPriority[p]) {
                        newPrimaryCall = voiceCalls.instance(i)
                        break
                    }
                }
                if (newPrimaryCall) {
                    break
                }
            }
            var person = "";
            if (newPrimaryCall && callerDetails[newPrimaryCall.handlerId]) {
                person = callerDetails[newPrimaryCall.handlerId].person
            }

            for (var ic = 0; ic < voiceCalls.count; ic++) {
                var call = voiceCalls.instance(ic)
                console.log("call: " + call.lineId + "  state: " + call.statusText + "   " + call.status + "   " + call.handlerId + "\n")
                if (call.status === VoiceCall.STATUS_ALERTING || call.status === VoiceCall.STATUS_DIALING) {
                    console.log("Tell outgoing: " + call.lineId);
                    //FIXME: Not working?
                    watchConnector.ring(call.lineId, person, 0);
                } else if (call.status === VoiceCall.STATUS_INCOMING || call.status === VoiceCall.STATUS_WAITING) {
                    console.log("Tell incoming: " + call.lineId);
                    watchConnector.ring(call.lineId, person);
                } else if (call.status === VoiceCall.STATUS_DISCONNECTED || call.status === VoiceCall.STATUS_NULL) {
                    console.log("Endphone");
                    watchConnector.endPhoneCall();
                } else if (call.status === VoiceCall.ACTIVE) {
                    console.log("Startphone");
                    watchConnector.startPhoneCall();
                }
            }
        }
    }
    Timer {
        id: updateStateTimer
        interval: 50
        onTriggered: manager.updateState()
    }

    // Receive the calls
    Instantiator {
         id: callMonitor
         model: manager.voiceCalls

         delegate: QtObject {
            property string callStatus: instance.status
            property string remoteUid
            property string person: ""
            onCallStatusChanged: {
                console.log("Status changed: " + callStatus);
                if (callStatus === VoiceCall.STATUS_DISCONNECTED || callStatus === VoiceCall.STATUS_NULL) {
                    watchConnector.endPhoneCall();
                    manager.updateState();
                } else {
                    updateStateTimer.start();
                }
            }
            Component.onCompleted: {
                remoteUid = instance.lineId
                person = remoteUid !== "" ? people.personByPhoneNumber(remoteUid) : ""
                manager.updateState()
            }
         }
         onObjectAdded: {
             callerDetails[object.handlerId] = object
         }
         onObjectRemoved: {
             delete callerDetails[object.handlerId]
             manager.updateState()
         }
    }

    // Handle SMS
    ClassZeroSMSModel {
        id: classZeroSMS
        property Component dialogComponent

        onNewMessage: {
            console.log("New message: " + text)
            /*
            if (dialogComponent === null) {
                dialogComponent = Qt.createComponent("pages/ClassZeroSMS.qml")
                if (dialogComponent.status === Component.Error)
                    console.log("ClassZeroSMS: ", dialogComponent.errorString())
            }

            var dialog = dialogComponent.createObject(mainWindow, { "messageToken": messageToken, "text": text })
            dialog.visibleChanged.connect(function() { if (!dialog.visible) { dialog.destroy() } })
            dialog.activate()
            */

            classZeroSMS.clear()
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
                title: "WaterWatch"
            }
            Label {
                visible: !watchConnector.isConnected
                text: "Waiting for watch...\nIf it can't be found plase\ncheck it's available and\npaired in Bluetooth settings."
                width: column.width
            }
            // Select the device
            Repeater {
                model: KnownDevicesModel { id: knownDevicesModel }
                delegate: ListItem {
                    id: pairedItem
                    visible: (model.paired && watchConnector.isConnected)
                    Label {
                        text: model.alias.length ? model.alias : model.address
                    }
                    onVisibleChanged: {
                        if (pairedItem.visible) {
                            // Connect with the device
                            watchConnector.deviceConnect(model.alias, model.address);
                        }
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


