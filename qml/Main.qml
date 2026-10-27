import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1080
    height: 740
    minimumWidth: 960
    minimumHeight: 700
    visible: true
    title: "Air2Win"
    color: "#fafafc"
    font.family: Qt.application.font.family
    font.pixelSize: 14
    property int page: 0
    readonly property bool receiverOn: ready || receiver.state === "starting"
    readonly property color activeColor: "#b6dfca"
    readonly property bool ready: receiver.state === "listening" || receiver.state === "streaming"
    readonly property string statusText: receiver.state === "listening" ? "Waiting for a device"
        : receiver.state === "streaming" ? "Mirroring"
        : receiver.state === "starting" ? "Starting AirPlay..."
        : receiver.state === "stopping" ? "Stopping..."
        : receiver.state === "error" ? "Check the log" : "Receiver off"

    component Caption: Text {
        color: "#858591"
        font.pixelSize: 12
        font.weight: Font.Medium
    }
    component BodyText: Text {
        color: "#777885"
        font.pixelSize: 14
        wrapMode: Text.WordWrap
        lineHeight: 1.25
    }

    Rectangle {
        id: sidebar
        width: 218
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        color: "#f1f2f4"
        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: "#e8e8ef" }
        Column {
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 22; topMargin: 34 }
            spacing: 8
            Row {
                spacing: 10
                Image {
                    width: 34; height: 34
                    source: "qrc:/branding/7629289.png"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }
                Text { text: "Air2Win"; color: "#22232d"; font.pixelSize: 23; font.weight: Font.DemiBold; anchors.verticalCenter: parent.verticalCenter }
            }
        }
        Column {
            anchors { top: parent.top; topMargin: 144; left: parent.left; right: parent.right; margins: 14 }
            spacing: 7
            Repeater {
                model: [{label: "AirPlay", icon: "screen"}, {label: "Settings", icon: "settings"}, {label: "Log", icon: "log"}]
                delegate: Button {
                    id: nav
                    required property var modelData
                    required property int index
                    width: parent.width; height: 44
                    hoverEnabled: true
                    onClicked: window.page = index
                    contentItem: Row {
                        spacing: 12; leftPadding: 14
                        Icon { kind: nav.modelData.icon; ink: window.page === nav.index ? "#007aff" : "#81818e"; anchors.verticalCenter: parent.verticalCenter; width: 20; height: 20 }
                        Text { text: nav.modelData.label; anchors.verticalCenter: parent.verticalCenter; color: window.page === nav.index ? "#0066d6" : "#6e6f7c"; font.pixelSize: 13; font.weight: window.page === nav.index ? Font.DemiBold : Font.Normal }
                    }
                    background: Rectangle {
                        radius: 11
                        color: window.page === nav.index ? "#e1ebf7" : nav.hovered ? "#e9e9f0" : sidebar.color
                        border.color: nav.activeFocus ? "#8cbbf2" : "transparent"
                        Behavior on color { ColorAnimation { duration: 180 } }
                    }
                }
            }
        }
        Column {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right; margins: 24 }
            spacing: 10
            Caption { text: "v1.0.0"; font.pixelSize: 9; font.letterSpacing: 1; topPadding: 8 }
        }
    }

    Item {
        anchors { left: sidebar.right; top: parent.top; right: parent.right; bottom: parent.bottom; margins: 38 }
        RowLayout {
            id: topBar
            width: parent.width
            Rectangle {
                implicitWidth: statusRow.implicitWidth + 24; implicitHeight: 30; radius: 15
                color: window.ready ? "#e6f4ee" : receiver.state === "error" ? "#fff0e6" : "#efeff4"
                Row {
                    id: statusRow; anchors.centerIn: parent; spacing: 7
                    Rectangle { width: 6; height: 6; radius: 3; color: window.ready ? "#3b9b74" : receiver.state === "error" ? "#c48349" : "#9999a6"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: window.statusText; color: window.ready ? "#328564" : receiver.state === "error" ? "#ac713c" : "#858591"; font.pixelSize: 11 }
                }
            }
        }

        ColumnLayout {
            opacity: window.page === 0 ? 1 : 0
            visible: opacity > 0
            enabled: window.page === 0
            transform: Translate { y: window.page === 0 ? 0 : 8; Behavior on y { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } } }
            Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
            anchors { top: topBar.bottom; topMargin: 24; left: parent.left; right: parent.right; bottom: parent.bottom }
            spacing: 18
            Text { text: "AirPlay"; color: "#242530"; font.pixelSize: 29; font.weight: Font.DemiBold; font.letterSpacing: -0.8; Layout.fillWidth: true }

            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 235
                radius: 22; clip: true
                color: window.receiverOn ? window.activeColor : "#eef0f3"
                Behavior on color { ColorAnimation { duration: 420; easing.type: Easing.InOutCubic } }
                Column {
                    anchors.centerIn: parent; spacing: 24
                    Image {
                        width: 96; height: 96
                        anchors.horizontalCenter: parent.horizontalCenter
                        source: "qrc:/branding/7629289.png"
                        fillMode: Image.PreserveAspectFit
                        mipmap: true
                        scale: window.receiverOn ? 1.06 : 1
                        Behavior on scale { NumberAnimation { duration: 420; easing.type: Easing.OutCubic } }
                    }
                    Text {
                        text: receiver.state === "streaming" ? "Mirroring" : window.ready ? "Ready to connect" : receiver.state === "starting" ? "Starting…" : receiver.state === "stopping" ? "Stopping…" : "Receiver is off"
                        color: "#283b33"; font.pixelSize: 19; font.weight: Font.Medium
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 12
                Column {
                    spacing: 5; Layout.fillWidth: true
                    Text { text: receiver.receiverName; color: "#292a36"; font.pixelSize: 17; font.weight: Font.DemiBold }
                }
                Text {
                    visible: window.ready && receiver.requirePin
                    text: "PIN  " + receiver.pin
                    color: "#557164"; font.pixelSize: 15; font.weight: Font.Medium
                    font.letterSpacing: 1
                }
                ActionButton {
                    outlineColor: window.receiverOn ? window.activeColor : "transparent"
                    text: receiver.active ? "Stop Receiver" : "Start AirPlay services"
                    secondary: receiver.active
                    enabled: receiver.state !== "starting" && receiver.state !== "stopping"
                    onClicked: receiver.active ? receiver.stop() : receiver.start()
                }
            }

            BodyText {
                visible: receiver.error.length > 0
                text: receiver.error; color: "#ae6e3f"; font.pixelSize: 12; Layout.fillWidth: true
            }
        }

        ColumnLayout {
            opacity: window.page === 1 ? 1 : 0
            visible: opacity > 0
            enabled: window.page === 1
            transform: Translate { y: window.page === 1 ? 0 : 8; Behavior on y { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } } }
            Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
            anchors { top: topBar.bottom; topMargin: 28; left: parent.left; right: parent.right }
            spacing: 22
            Text { text: "Receiver Settings"; color: "#242530"; font.pixelSize: 29; font.weight: Font.DemiBold; font.letterSpacing: -0.8 }
            BodyText { text: "Configuration is stored locally in the application directory."; Layout.fillWidth: true }
            Rectangle {
                Layout.fillWidth: true; implicitHeight: settingsColumn.implicitHeight + 48; radius: 18; color: "white"; border.color: "#e8e8ef"
                ColumnLayout {
                    id: settingsColumn
                    anchors { left: parent.left; right: parent.right; top: parent.top; margins: 24 }
                    spacing: 14
                    Text { text: "AirPlay Receiver Name"; color: "#353642"; font.weight: Font.DemiBold }
                    TextField {
                        id: nameField; Layout.fillWidth: true; implicitHeight: 45; text: receiver.receiverName
                        enabled: !receiver.active; selectByMouse: true; leftPadding: 14; color: "#343541"
                        background: Rectangle { radius: 10; color: "#f7f7fa"; border.color: nameField.activeFocus ? "#8cbbf2" : "#e5e5ed" }
                    }
                    CheckBox { id: pinCheck; text: "Require PIN on connection"; checked: receiver.requirePin; enabled: !receiver.active }
                    BodyText { text: "The PIN appears on the main screen and changes each time the receiver starts."; font.pixelSize: 12; Layout.fillWidth: true }
                    RowLayout {
                        ActionButton { text: "Save changes"; enabled: !receiver.active; onClicked: { if (receiver.saveSettings(nameField.text, pinCheck.checked)) savedLabel.text = "Saved" } }
                        Caption { id: savedLabel; text: receiver.active ? "Stop receiver before editing" : "" }
                    }
                }
            }
            BodyText { text: "Screen and audio via UxPlay. DRM content and direct app video casting are not supported yet."; Layout.fillWidth: true; font.pixelSize: 12 }
            BodyText { visible: receiver.error.length > 0; text: receiver.error; color: "#ae6e3f"; Layout.fillWidth: true }
        }

        ColumnLayout {
            opacity: window.page === 2 ? 1 : 0
            visible: opacity > 0
            enabled: window.page === 2
            transform: Translate { y: window.page === 2 ? 0 : 8; Behavior on y { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } } }
            Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
            anchors { top: topBar.bottom; topMargin: 28; left: parent.left; right: parent.right; bottom: parent.bottom }
            spacing: 18
            Text { text: "Diagnostic Log"; color: "#242530"; font.pixelSize: 29; font.weight: Font.DemiBold; font.letterSpacing: -0.8 }
            BodyText { text: "Local addresses: " + receiver.localAddresses; Layout.fillWidth: true }
            BodyText { text: "If your iPhone or iPad cannot find the PC, check the network profile and Windows Firewall rules. VPNs and guest networks may block discovery."; Layout.fillWidth: true; font.pixelSize: 12 }
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; radius: 16; color: "#f1f2f4"; border.color: "#e5e5ec"
                ScrollView {
                    anchors.fill: parent; anchors.margins: 16; clip: true
                    TextArea {
                        readOnly: true; selectByMouse: true; text: receiver.diagnostics.length ? receiver.diagnostics : "Diagnostic log will appear here once the receiver starts"
                        font.family: "Consolas"; font.pixelSize: 11; color: "#5d5e70"; wrapMode: TextEdit.Wrap
                        background: null
                    }
                }
            }
            RowLayout {
                Caption { text: "Buffer limit: last 200 lines\nStored in memory only"; Layout.fillWidth: true }
                ActionButton { text: "Clear log"; secondary: true; onClicked: receiver.clearDiagnostics() }
            }
        }
    }
}
