import QtQuick

Canvas {
    id: icon
    property string kind: "screen"
    property color ink: "#747481"
    onInkChanged: requestPaint()
    onKindChanged: requestPaint()
    implicitWidth: 24
    implicitHeight: 24
    onPaint: {
        let c = getContext("2d")
        c.reset()
        c.scale(width / 24, height / 24)
        c.strokeStyle = ink
        c.fillStyle = ink
        c.lineWidth = 1.6
        c.lineCap = "round"
        c.lineJoin = "round"
        if (kind === "screen") {
            c.beginPath(); c.moveTo(7, 19); c.lineTo(12, 14); c.lineTo(17, 19); c.closePath(); c.fill()
            c.beginPath(); c.moveTo(5, 17); c.lineTo(3, 17); c.lineTo(3, 4); c.lineTo(21, 4); c.lineTo(21, 17); c.lineTo(19, 17); c.stroke()
        } else if (kind === "settings") {
            for (let y of [6,12,18]) { c.beginPath(); c.moveTo(4,y); c.lineTo(20,y); c.stroke() }
            for (let p of [[9,6],[15,12],[8,18]]) {
                c.fillStyle = "#f3f3f7"; c.beginPath(); c.arc(p[0],p[1],2.5,0,Math.PI*2); c.fill(); c.stroke()
            }
        } else if (kind === "log") {
            c.strokeRect(4,3,16,18)
            for (let y of [8,12,16]) { c.beginPath(); c.moveTo(8,y); c.lineTo(y===16?13:16,y); c.stroke() }
        } else if (kind === "shield") {
            c.beginPath(); c.moveTo(12,3); c.lineTo(20,6); c.lineTo(19,14); c.quadraticCurveTo(18,18,12,21); c.quadraticCurveTo(6,18,5,14); c.lineTo(4,6); c.closePath(); c.stroke()
            c.beginPath(); c.moveTo(8,12); c.lineTo(11,15); c.lineTo(16,9); c.stroke()
        } else if (kind === "wifi") {
            for (let r of [5,9,13]) { c.beginPath(); c.arc(12,21,r,Math.PI*1.24,Math.PI*1.76); c.stroke() }
            c.beginPath(); c.arc(12,20,1,0,Math.PI*2); c.fill()
        }
    }
}
