import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: player
    objectName: "nowPlayingWindow"
    title: "Now Playing — Air2Win"
    width: 400
    height: 650
    minimumWidth: 400
    maximumWidth: 400
    minimumHeight: 650
    maximumHeight: 650
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowMinimizeButtonHint | Qt.WindowCloseButtonHint
    color: "#faf8f9"
    readonly property bool playing: nowPlaying.controls.stateKnown ? nowPlaying.controls.playing : nowPlaying.receiving
    property int frontCover: -1
    property int pendingCover: -1
    onVisibleChanged: {
        nowPlaying.controls.setMonitoring(visible)
        if (visible) { opening.restart(); detailsReveal.restart() }
    }

    function stageCover(source) {
        fadeA.stop(); fadeB.stop()
        if (!source.length) {
            frontCover = -1; pendingCover = -1
            coverA.source = ""; coverB.source = ""
            coverA.opacity = 0; coverB.opacity = 0
            return
        }
        pendingCover = frontCover === 0 ? 1 : 0
        const incoming = pendingCover === 0 ? coverA : coverB
        incoming.opacity = 0
        incoming.z = 1
        const previous = pendingCover === 0 ? coverB : coverA
        previous.z = 0
        if (frontCover >= 0) previous.opacity = 1
        incoming.source = source
        if (incoming.status === Image.Ready) activateCover(pendingCover)
    }
    function activateCover(slot) {
        if (slot !== pendingCover || slot === frontCover) return
        frontCover = slot
        if (!visible) {
            const incoming = slot === 0 ? coverA : coverB
            incoming.opacity = 1
            finishCover(slot)
            return
        }
        if (slot === 0) fadeA.restart()
        else fadeB.restart()
    }
    function finishCover(slot) {
        if (frontCover !== slot) return
        const previous = slot === 0 ? coverB : coverA
        previous.opacity = 0
        previous.source = ""
    }

    component TransportButton: Button {
        id: control
        required property string symbol
        implicitWidth: 48
        implicitHeight: 48
        hoverEnabled: true
        opacity: enabled ? 1 : 0.4
        scale: down ? 0.9 : 1
        Behavior on scale { ScaleAnimator { duration: 180; easing.type: Easing.OutCubic } }
        Behavior on opacity { OpacityAnimator { duration: 140 } }
        onSymbolChanged: glyphReveal.restart()
        contentItem: Item {
            id: glyph
            Icon { anchors.centerIn: parent; width: 24; height: 24; kind: control.symbol; ink: "#64515e" }
        }
        OpacityAnimator { id: glyphReveal; target: glyph; from: 0.4; to: 1; duration: 160 }
        background: Rectangle {
            radius: 24
            color: "#ede5eb"
            opacity: control.down ? 1 : control.hovered || control.activeFocus ? 0.7 : 0
            Behavior on opacity { OpacityAnimator { duration: 140; easing.type: Easing.InOutSine } }
        }
        ToolTip.visible: hovered
        ToolTip.delay: 600
        ToolTip.text: text
    }

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
        function onArtworkChanged() { player.stageCover(nowPlaying.artwork) }
        function onMetadataChanged() { if (player.visible) detailsReveal.restart() }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0
                color: Qt.tint("#faf8f9", Qt.rgba(nowPlaying.accent.r, nowPlaying.accent.g, nowPlaying.accent.b, 0.14))
                Behavior on color { ColorAnimation { duration: 420; easing.type: Easing.InOutCubic } }
            }
            GradientStop { position: 0.8; color: "#faf8f9" }
        }
    }

    ColumnLayout {
        id: content
        anchors { fill: parent; margins: 28 }
        spacing: 0

        Item {
            id: coverSurface
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 320
            Layout.preferredHeight: 320
            scale: player.playing ? 1 : 0.96
            Behavior on scale { ScaleAnimator { duration: 380; easing.type: Easing.OutCubic } }
            Rectangle {
                anchors.fill: parent
                radius: 16
                color: "#eeeaed"
                Text {
                    anchors.centerIn: parent
                    text: "\u266a"
                    font.pixelSize: 88
                    color: "#b1a6af"
                }
            }
            Image {
                id: coverA
                anchors.fill: parent
                sourceSize: Qt.size(688, 688)
                asynchronous: true
                cache: false
                fillMode: Image.PreserveAspectFit
                opacity: 0
                onStatusChanged: if (status === Image.Ready) player.activateCover(0)
            }
            Image {
                id: coverB
                anchors.fill: parent
                sourceSize: Qt.size(688, 688)
                asynchronous: true
                cache: false
                fillMode: Image.PreserveAspectFit
                opacity: 0
                onStatusChanged: if (status === Image.Ready) player.activateCover(1)
            }
            OpacityAnimator { id: fadeA; target: coverA; from: 0; to: 1; duration: 320; easing.type: Easing.InOutCubic; onFinished: player.finishCover(0) }
            OpacityAnimator { id: fadeB; target: coverB; from: 0; to: 1; duration: 320; easing.type: Easing.InOutCubic; onFinished: player.finishCover(1) }
        }

        Item { Layout.preferredHeight: 25 }
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: detailsColumn.implicitHeight
            ColumnLayout {
                id: detailsColumn
                width: parent.width
                spacing: 4
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
                    text: nowPlaying.artist
                    textFormat: Text.PlainText
                    visible: text.length > 0
                    font.pixelSize: 16
                    color: "#6a5966"
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: nowPlaying.album
                    textFormat: Text.PlainText
                    visible: text.length > 0
                    font.pixelSize: 13
                    color: "#94858f"
                    elide: Text.ElideRight
                }
            }
        }
        Item { Layout.fillHeight: true; Layout.minimumHeight: 16 }

        ColumnLayout {
            Layout.fillWidth: true
            visible: nowPlaying.duration > 0
            spacing: 8
            Slider {
                id: seekBar
                Layout.fillWidth: true
                implicitHeight: 18
                from: 0
                to: Math.max(1, nowPlaying.duration)
                enabled: nowPlaying.controls.canSeek && !nowPlaying.controls.busy
                property real dragPosition: 0
                property bool awaitingPosition: false
                function submit(position) {
                    if (!enabled) return
                    dragPosition = position
                    awaitingPosition = true
                    confirmation.restart()
                    nowPlaying.controls.seek(Math.round(position))
                }
                Binding { target: seekBar; property: "value"; value: Math.min(seekBar.to, nowPlaying.position); when: !seekBar.pressed && !seekBar.awaitingPosition; restoreMode: Binding.RestoreNone }
                Timer { id: confirmation; interval: 2500; onTriggered: seekBar.awaitingPosition = false }
                Connections {
                    target: nowPlaying
                    function onProgressChanged() {
                        if (seekBar.awaitingPosition && Math.abs(nowPlaying.position - seekBar.dragPosition) <= 2) {
                            confirmation.stop()
                            seekBar.awaitingPosition = false
                        }
                    }
                }
                Connections {
                    target: nowPlaying.controls
                    function onChanged() {
                        if (!nowPlaying.controls.available || nowPlaying.controls.error.length > 0) {
                            confirmation.stop()
                            seekBar.awaitingPosition = false
                        }
                    }
                }
                onPressedChanged: {
                    if (pressed) dragPosition = value
                    else submit(dragPosition)
                }
                onMoved: {
                    dragPosition = value
                    if (!pressed) submit(value)
                }
                background: Rectangle {
                    x: seekBar.leftPadding; y: (seekBar.height - height) / 2
                    width: seekBar.availableWidth; height: 3; radius: 1.5; color: "#e4dfe3"
                    Rectangle { width: parent.width * seekBar.visualPosition; height: 3; radius: 1.5; color: "#8c7885" }
                }
                handle: Rectangle {
                    x: seekBar.leftPadding + seekBar.visualPosition * (seekBar.availableWidth - width)
                    y: (seekBar.height - height) / 2
                    width: 9; height: 9; radius: 4.5; color: "#8c7885"
                    visible: seekBar.enabled && (seekBar.hovered || seekBar.pressed || seekBar.activeFocus)
                }
                Accessible.name: "Playback position"
            }
            RowLayout {
                Layout.fillWidth: true
                Text { text: player.clock(seekBar.pressed ? seekBar.value : nowPlaying.position); font.pixelSize: 11; color: "#968c93"; Layout.fillWidth: true }
                Text { text: "\u2212" + player.clock(Math.max(0, nowPlaying.duration - nowPlaying.position)); font.pixelSize: 11; color: "#968c93" }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 8
            spacing: 22
            visible: nowPlaying.controls.available
            enabled: !nowPlaying.controls.busy
            TransportButton { symbol: "previous"; text: "Previous track"; onClicked: nowPlaying.controls.previous() }
            TransportButton {
                symbol: (nowPlaying.controls.stateKnown ? nowPlaying.controls.playing : nowPlaying.receiving) ? "pause" : "play"
                text: symbol === "pause" ? "Pause" : "Play"
                onClicked: nowPlaying.controls.playPause()
            }
            TransportButton { symbol: "next"; text: "Next track"; onClicked: nowPlaying.controls.next() }
        }

        Text {
            Layout.fillWidth: true
            visible: nowPlaying.controls.error.length > 0
            text: nowPlaying.controls.error
            textFormat: Text.PlainText
            color: "#a36363"
            font.pixelSize: 11
            elide: Text.ElideRight
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
    OpacityAnimator { id: opening; target: content; from: 0; to: 1; duration: 180; easing.type: Easing.OutCubic }
    ParallelAnimation {
        id: detailsReveal
        OpacityAnimator { target: detailsColumn; from: 0.2; to: 1; duration: 220; easing.type: Easing.OutCubic }
        YAnimator { target: detailsColumn; from: 5; to: 0; duration: 280; easing.type: Easing.OutCubic }
    }

}
