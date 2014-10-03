import QtQuick 2.0
import Sailfish.Silica 1.0
import "pages"
import harbour.org.pebbled 0.1

ApplicationWindow
{
    initialPage: Component { ManagerPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    PebbledInterface {
        id: pebbled
    }
}
