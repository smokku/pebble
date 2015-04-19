import QtQuick 2.0
import QtQml 2.1
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
            spacing: Theme.paddingMedium

            PageHeader {
                title: "pebbled"
            }
            Label {
                text: qsTr("Version") + " " + APP_VERSION
                horizontalAlignment: Text.AlignRight
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }
            }
            Label {
                color: Theme.highlightColor
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: "© 2014-2015 Tomasz Sterna / Xiaoka.com\n" + qsTr("All Rights Reserved.")
            }
            Label {
                wrapMode: Text.Wrap
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingMedium
                }
                font.pixelSize: Theme.fontSizeTiny
                horizontalAlignment: Text.AlignJustify
                text: (
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND "+
"ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED "+
"WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE "+
"DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR "+
"ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES "+
"(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; "+
"LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND "+
"ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT "+
"(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS "+
"SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.")
            }
            Label {
                text: qsTr("Support")
                font.family: Theme.fontFamilyHeading
                color: Theme.highlightColor
                anchors.right: parent.right
                anchors.rightMargin: Theme.paddingMedium
            }
            Label {
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
                text: qsTr("Your donations help justify development time.")
            }
            Label {
                visible: !!donate.active
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }
                font.pixelSize: Theme.fontSizeLarge
                font.italic: true
                color: Theme.highlightColor
                wrapMode: Text.Wrap
                text: qsTr("Thank you for your support!!!")
            }
            Button {
                text: qsTr("PayPal Donate")
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge * 2
                }
                onClicked: Qt.openUrlExternally(donate.url)
            }

            Label {
                text: qsTr("Bugs?")
                font.family: Theme.fontFamilyHeading
                color: Theme.highlightColor
                anchors.right: parent.right
                anchors.rightMargin: Theme.paddingMedium
            }
            Button {
                text: qsTr("Open Bug Tracker")
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge * 2
                }
                onClicked: Qt.openUrlExternally("https://github.com/smokku/pebble/issues")
            }
            Button {
                text: qsTr("Forum Thread")
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge * 2
                }
                onClicked: Qt.openUrlExternally("http://talk.maemo.org/showthread.php?t=93399")
            }
            Button {
                text: qsTr("Send issue e-mail to developer")
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge * 2
                }
                onClicked: Qt.openUrlExternally("mailto:bugs@xiaoka.com?subject=pebbled issue&body=describe your issue here")
            }
        }
    }
}
