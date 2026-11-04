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
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowMinimizeButtonHint | Qt.WindowCloseButtonHint | (pinned ? Qt.WindowStaysOnTopHint : 0)
    color: "#23252c"
    property bool pinned: false
    signal showReceiverRequested()
    readonly property color primaryInk: "#ffffff"
    readonly property color secondaryInk: "#c4ffffff"
    readonly property color mutedInk: "#94ffffff"
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
            backgroundA.source = ""; backgroundB.source = ""
            backgroundA.opacity = 0; backgroundB.opacity = 0
            return
        }
        pendingCover = frontCover === 0 ? 1 : 0
        const incoming = pendingCover === 0 ? coverA : coverB
        incoming.opacity = 0
        incoming.z = 1
        const previous = pendingCover === 0 ? coverB : coverA
        previous.z = 0
        if (frontCover >= 0) previous.opacity = 1
        const backdrop = pendingCover === 0 ? backgroundA : backgroundB
        const oldBackdrop = pendingCover === 0 ? backgroundB : backgroundA
        backdrop.opacity = 0; backdrop.z = 1; oldBackdrop.z = 0
        if (frontCover >= 0) oldBackdrop.opacity = 1
        backdrop.source = nowPlaying.background
        // Prepare both layers before a cached image can report Ready synchronously.
        incoming.source = source
        if (incoming.status === Image.Ready) activateCover(pendingCover)
    }
    function activateCover(slot) {
        if (slot !== pendingCover || slot === frontCover) return
        frontCover = slot
        if (!visible) {
            const incoming = slot === 0 ? coverA : coverB
            incoming.opacity = 1
            const backdrop = slot === 0 ? backgroundA : backgroundB
            backdrop.opacity = 1
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
        const previousBackdrop = slot === 0 ? backgroundB : backgroundA
        previousBackdrop.opacity = 0; previousBackdrop.source = ""
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
            Icon { anchors.centerIn: parent; width: 24; height: 24; kind: control.symbol; ink: player.primaryInk }
        }
        OpacityAnimator { id: glyphReveal; target: glyph; from: 0.4; to: 1; duration: 160 }
        background: Rectangle {
            radius: 24
            color: "#24ffffff"
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
        function onMetadataChanged() {
            confirmation.stop()
            seekBar.awaitingPosition = false
            if (player.visible) detailsReveal.restart()
        }
    }

    Item {
        anchors.fill: parent
        Image { id: backgroundA; anchors.fill: parent; smooth: true; cache: false; opacity: 0 }
        Image { id: backgroundB; anchors.fill: parent; smooth: true; cache: false; opacity: 0 }
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
                // A backing rectangle leaks through filtered artwork edges at fractional DPI.
                visible: player.frontCover < 0
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
            ParallelAnimation {
                id: fadeA
                OpacityAnimator { target: coverA; from: 0; to: 1; duration: 420; easing.type: Easing.InOutCubic }
                OpacityAnimator { target: backgroundA; from: 0; to: 1; duration: 420; easing.type: Easing.InOutCubic }
                onFinished: player.finishCover(0)
            }
            ParallelAnimation {
                id: fadeB
                OpacityAnimator { target: coverB; from: 0; to: 1; duration: 420; easing.type: Easing.InOutCubic }
                OpacityAnimator { target: backgroundB; from: 0; to: 1; duration: 420; easing.type: Easing.InOutCubic }
                onFinished: player.finishCover(1)
            }
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
                    color: player.primaryInk
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
                    color: player.secondaryInk
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: nowPlaying.album
                    textFormat: Text.PlainText
                    visible: text.length > 0
                    font.pixelSize: 13
                    color: player.mutedInk
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
                    width: seekBar.availableWidth; height: 3; radius: 1.5; color: "#40ffffff"
                    Rectangle { width: parent.width * seekBar.visualPosition; height: 3; radius: 1.5; color: player.secondaryInk }
                }
                handle: Rectangle {
                    x: seekBar.leftPadding + seekBar.visualPosition * (seekBar.availableWidth - width)
                    y: (seekBar.height - height) / 2
                    width: 9; height: 9; radius: 4.5; color: player.secondaryInk
                    visible: seekBar.enabled && (seekBar.hovered || seekBar.pressed || seekBar.activeFocus)
                }
                Accessible.name: "Playback position"
            }
            RowLayout {
                Layout.fillWidth: true
                Text { text: player.clock(seekBar.pressed ? seekBar.value : nowPlaying.position); font.pixelSize: 11; color: player.mutedInk; Layout.fillWidth: true }
                Text { text: "\u2212" + player.clock(Math.max(0, nowPlaying.duration - nowPlaying.position)); font.pixelSize: 11; color: player.mutedInk }
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
            color: "#ffcdcd"
            font.pixelSize: 11
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 20
            spacing: 7
            Button {
                text: "Air2Win"
                implicitHeight: 30
                leftPadding: 0; rightPadding: 8
                contentItem: Text { text: parent.text; color: player.secondaryInk; font.pixelSize: 12; verticalAlignment: Text.AlignVCenter }
                background: Item {}
                onClicked: player.showReceiverRequested()
                ToolTip.visible: hovered
                ToolTip.text: "Open receiver"
                Layout.fillWidth: true
            }
            Rectangle { implicitWidth: 5; implicitHeight: 5; radius: 2.5; color: nowPlaying.receiving ? "#5d9b79" : "#b3a9b0" }
            Text { text: nowPlaying.receiving ? "Receiving audio" : "Connected"; font.pixelSize: 11; color: player.mutedInk }
            Button {
                id: pinButton
                objectName: "pinButton"
                checkable: true
                checked: player.pinned
                onToggled: player.pinned = checked
                implicitWidth: 30; implicitHeight: 30
                contentItem: Icon { kind: "pin"; ink: player.pinned ? player.primaryInk : player.mutedInk }
                background: Rectangle { radius: 8; color: pinButton.checked ? "#30ffffff" : pinButton.hovered ? "#18ffffff" : "transparent" }
                Accessible.name: player.pinned ? "Unpin window" : "Keep on top"
                ToolTip.visible: hovered
                ToolTip.text: player.pinned ? "Unpin window" : "Keep on top"
            }
        }
    }
    OpacityAnimator { id: opening; target: content; from: 0; to: 1; duration: 180; easing.type: Easing.OutCubic }
    ParallelAnimation {
        id: detailsReveal
        OpacityAnimator { target: detailsColumn; from: 0.2; to: 1; duration: 220; easing.type: Easing.OutCubic }
        YAnimator { target: detailsColumn; from: 5; to: 0; duration: 280; easing.type: Easing.OutCubic }
    }

}
