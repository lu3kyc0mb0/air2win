import QtQuick
import QtQuick.Controls

Button {
    id: control
    property bool secondary: false
    property color outlineColor: "transparent"
    scale: down ? 0.98 : 1
    Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
    implicitHeight: 46
    implicitWidth: Math.max(140, contentItem.implicitWidth + 44)
    hoverEnabled: true
    font.family: Qt.application.font.family
    font.pixelSize: 14
    font.weight: Font.DemiBold
    contentItem: Text {
        text: control.text
        font: control.font
        color: !control.enabled ? "#a3a4ad" : control.secondary ? "#303240" : "white"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 13
        color: !control.enabled ? "#e9e9ee" : control.secondary
               ? (control.hovered ? "#e5e6ed" : "#efeff4")
               : (control.down ? "#005ec4" : control.hovered ? "#268bfa" : "#007aff")
        border.color: control.activeFocus ? "#8cbbf2" : control.outlineColor
        border.width: 2
        Behavior on color { ColorAnimation { duration: 180 } }
        Behavior on border.color { ColorAnimation { duration: 420; easing.type: Easing.InOutCubic } }
    }
}
