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
                text: qsTr("Version ") + APP_VERSION
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
                text: "© 2014 Tomasz Sterna / Xiaoka.com\nAll Rights Reserved."
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
                text: qsTr(
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
            IconButton {
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingMedium
                }
                icon.source: "../images/btn_donate.png"
                height: icon.height - Theme.paddingLarge
                onClicked: Qt.openUrlExternally("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=MAGN86VCARBSA")
            }

            Label {
                text: qsTr("Bugs?")
                font.family: Theme.fontFamilyHeading
                color: Theme.highlightColor
                anchors.right: parent.right
                anchors.rightMargin: Theme.paddingMedium
            }
            Button {
                text: "Open Bug Tracker"
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }
                onClicked: Qt.openUrlExternally("https://github.com/smokku/pebble/issues")
            }
            Button {
                text: "Send application log to developer"
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }
                onClicked: Qt.openUrlExternally("mailto:bugs@xiaoka.com?subject=pebbled issue&body=please describe your issue&attachment=/home/nemo/.cache/pebbled/pebble.log")
            }
        }
    }
}
