import QtQuick 2.15
import QtQuick.Controls 2.15

ApplicationWindow {
    id: root
    visible: true
    width: 900
    height: 600
    title: "Balance - Encryption Toolkit"

    // Glass-like background
    Rectangle {
        anchors.fill: parent
        color: "#F0F0F5"
        opacity: 0.95
    }

    Text {
        anchors.centerIn: parent
        text: "Balance QML Interface\n(Phase 2 Enhancement)"
        font.pixelSize: 24
        color: "#333"
    }
}
