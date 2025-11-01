import QtQuick 2.15
import QtQuick.Controls 2.15

Window {
    id: chatWindow
    width: 600
    height: 500
    title: "Balance Chat"

    Rectangle {
        anchors.fill: parent
        color: "#F0F0F5"
        opacity: 0.95
    }

    Text {
        anchors.centerIn: parent
        text: "Chat Window\n(Phase 2 Implementation)"
        font.pixelSize: 20
        color: "#333"
    }
}
