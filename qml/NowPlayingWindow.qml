import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

Window {
    id: player
    objectName: "nowPlayingWindow"
    title: "Now Playing — Air2Win"
    width: 400
    height: 598
    minimumWidth: 360
    minimumHeight: 560
    color: "#faf8f9"

    function present() {
        show()
        raise()
        requestActivate()
    }
    function clock(seconds) {
        return Math.floor(seconds / 60) + ":" + (Math.floor(seconds) % 60).toString().padStart(2, "0")
    }

    Connections {
        target: nowPlaying
        function onAvailableChanged() {
            if (nowPlaying.available) player.present()
            else player.close()
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.tint("#faf8f9", Qt.rgba(nowPlaying.accent.r, nowPlaying.accent.g, nowPlaying.accent.b, 0.14)) }
            GradientStop { position: 0.8; color: "#faf8f9" }
        }
    }

    ColumnLayout {
        anchors { fill: parent; margins: 28 }
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: width
            Rectangle {
                anchors.fill: parent
                radius: 16
                color: "#eeeaed"
                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: "#30000000"
                    shadowBlur: 0.5
                    shadowVerticalOffset: 7
                }
                Text {
                    anchors.centerIn: parent
                    text: "\u266a"
                    font.pixelSize: 88
                    color: "#b1a6af"
                }
            }
            Image {
                id: cover
                anchors.fill: parent
                source: nowPlaying.artwork
                sourceSize: Qt.size(720, 720)
                asynchronous: true
                cache: false
                fillMode: Image.PreserveAspectCrop
                visible: false
                onStatusChanged: if (status === Image.Ready) reveal.restart()
            }
            Rectangle {
                id: coverMask
                anchors.fill: parent
                radius: 16
                layer.enabled: true
                visible: false
            }
            MultiEffect {
                id: coverEffect
                anchors.fill: parent
                source: cover
                visible: cover.status === Image.Ready
                maskEnabled: true
                maskSource: coverMask
                autoPaddingEnabled: false
                maskThresholdMin: 0.5
                maskSpreadAtMin: 0.01
            }
            NumberAnimation { id: reveal; target: coverEffect; property: "opacity"; from: 0; to: 1; duration: 260; easing.type: Easing.OutCubic }
        }

        Item { Layout.preferredHeight: 25 }
        Text {
            Layout.fillWidth: true
            text: nowPlaying.title || "AirPlay audio"
            textFormat: Text.PlainText
            font.family: Qt.application.font.family
            font.pixelSize: 23
            font.weight: Font.DemiBold
            font.letterSpacing: -0.5
            color: "#282329"
            maximumLineCount: 2
            wrapMode: Text.Wrap
            elide: Text.ElideRight
        }
        Text {
            Layout.fillWidth: true
            Layout.topMargin: 5
            text: nowPlaying.artist
            textFormat: Text.PlainText
            visible: text.length > 0
            font.pixelSize: 16
            color: "#6a5966"
            elide: Text.ElideRight
        }
        Text {
            Layout.fillWidth: true
            Layout.topMargin: 3
            text: nowPlaying.album
            textFormat: Text.PlainText
            visible: text.length > 0
            font.pixelSize: 13
            color: "#94858f"
            elide: Text.ElideRight
        }
        Item { Layout.fillHeight: true; Layout.minimumHeight: 16 }

        ColumnLayout {
            Layout.fillWidth: true
            visible: nowPlaying.duration > 0
            spacing: 8
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 3
                radius: 1.5
                color: "#e4dfe3"
                Rectangle {
                    width: parent.width * Math.min(1, nowPlaying.position / Math.max(1, nowPlaying.duration))
                    height: parent.height
                    radius: parent.radius
                    color: "#8c7885"
                    Behavior on width { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Text { text: player.clock(nowPlaying.position); font.pixelSize: 11; color: "#968c93"; Layout.fillWidth: true }
                Text { text: "\u2212" + player.clock(Math.max(0, nowPlaying.duration - nowPlaying.position)); font.pixelSize: 11; color: "#968c93" }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 20
            spacing: 7
            Image { source: "qrc:/branding/7629289.png"; Layout.preferredWidth: 15; Layout.preferredHeight: 15; mipmap: true }
            Text { text: "AirPlay"; font.pixelSize: 12; font.weight: Font.Medium; color: "#93818c"; Layout.fillWidth: true }
            Rectangle { implicitWidth: 5; implicitHeight: 5; radius: 2.5; color: nowPlaying.receiving ? "#5d9b79" : "#b3a9b0" }
            Text { text: nowPlaying.receiving ? "Receiving audio" : "Connected"; font.pixelSize: 11; color: "#9d919a" }
        }
    }
}
