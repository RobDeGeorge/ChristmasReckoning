// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes
import ChristmasReckoning
import "../js/fmt.js" as Fmt

// Minimal line chart: optional log axes, area fill, a "now" marker and labelled ends.
Item {
    id: chart
    property var points: []          // [{x, y}]
    property bool xLog: false
    property bool yLog: false
    property string title
    property var xFormat: v => Fmt.num(v)
    property var yFormat: v => Fmt.num(v)
    property real markerX: NaN
    property color lineColor: Theme.gold
    property bool fill: true
    property bool showPoints: false

    readonly property int padLeft: 64
    readonly property int padBottom: 22
    readonly property int padTop: title.length ? 22 : 6
    readonly property real plotW: Math.max(10, width - padLeft - 8)
    readonly property real plotH: Math.max(10, height - padBottom - padTop)

    function tx(v) { return xLog ? Math.log10(Math.max(v, 1e-300)) : v }
    function ty(v) { return yLog ? Math.log10(Math.max(v, 1e-300)) : v }
    readonly property var bounds: {
        let x0 = Infinity, x1 = -Infinity, y0 = Infinity, y1 = -Infinity;
        for (const p of points) {
            x0 = Math.min(x0, tx(p.x)); x1 = Math.max(x1, tx(p.x));
            y0 = Math.min(y0, ty(p.y)); y1 = Math.max(y1, ty(p.y));
        }
        if (!isFinite(x0)) return { x0: 0, x1: 1, y0: 0, y1: 1 };
        if (!yLog) y0 = Math.min(0, y0);
        if (y1 === y0) y1 = y0 + 1;
        if (x1 === x0) x1 = x0 + 1;
        return { x0: x0, x1: x1, y0: y0, y1: y1 };
    }
    function px(v) { return padLeft + (tx(v) - bounds.x0) / (bounds.x1 - bounds.x0) * plotW }
    function py(v) { return padTop + plotH - (ty(v) - bounds.y0) / (bounds.y1 - bounds.y0) * plotH }

    implicitHeight: 200
    Accessible.role: Accessible.Chart
    Accessible.name: title

    Label {
        visible: chart.title.length > 0
        width: chart.width
        text: chart.title
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall
        elide: Text.ElideRight
    }

    // Axes
    Rectangle { x: chart.padLeft; y: chart.padTop; width: 1; height: chart.plotH; color: Theme.lineStrong }
    Rectangle { x: chart.padLeft; y: chart.padTop + chart.plotH; width: chart.plotW; height: 1; color: Theme.lineStrong }

    Repeater { // y labels: bottom, middle, top
        model: 3
        delegate: Label {
            required property int index
            readonly property real value: chart.yLog ? Math.pow(10, chart.bounds.y0 + index / 2 * (chart.bounds.y1 - chart.bounds.y0))
                                                     : chart.bounds.y0 + index / 2 * (chart.bounds.y1 - chart.bounds.y0)
            x: 0
            width: chart.padLeft - 6
            y: chart.padTop + chart.plotH * (1 - index / 2) - height / 2
            text: chart.yFormat(value)
            color: Theme.textFaint
            font.family: Theme.monoFamily
            font.pixelSize: 10
            horizontalAlignment: Text.AlignRight
        }
    }
    Repeater { // x labels: left, middle, right
        model: 3
        delegate: Label {
            required property int index
            readonly property real value: chart.xLog ? Math.pow(10, chart.bounds.x0 + index / 2 * (chart.bounds.x1 - chart.bounds.x0))
                                                     : chart.bounds.x0 + index / 2 * (chart.bounds.x1 - chart.bounds.x0)
            x: Math.max(0, Math.min(chart.width - width, chart.padLeft + chart.plotW * index / 2 - width / 2))
            y: chart.padTop + chart.plotH + 5
            text: chart.xFormat(value)
            color: Theme.textFaint
            font.family: Theme.monoFamily
            font.pixelSize: 10
        }
    }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeWidth: 0
            strokeColor: "transparent"
            fillColor: chart.fill ? Qt.alpha(chart.lineColor, 0.16) : "transparent"
            startX: chart.points.length ? chart.px(chart.points[0].x) : 0
            startY: chart.padTop + chart.plotH
            PathPolyline {
                path: {
                    const pts = [Qt.point(chart.points.length ? chart.px(chart.points[0].x) : 0, chart.padTop + chart.plotH)];
                    for (const p of chart.points) pts.push(Qt.point(chart.px(p.x), chart.py(p.y)));
                    if (chart.points.length)
                        pts.push(Qt.point(chart.px(chart.points[chart.points.length - 1].x), chart.padTop + chart.plotH));
                    return pts;
                }
            }
        }
        ShapePath {
            strokeWidth: 2
            strokeColor: chart.lineColor
            fillColor: "transparent"
            joinStyle: ShapePath.RoundJoin
            PathPolyline {
                path: chart.points.map(p => Qt.point(chart.px(p.x), chart.py(p.y)))
            }
        }
    }

    Repeater {
        model: chart.showPoints ? chart.points : []
        delegate: Rectangle {
            required property var modelData
            x: chart.px(modelData.x) - 3
            y: chart.py(modelData.y) - 3
            width: 6
            height: 6
            radius: 3
            color: chart.lineColor
        }
    }

    Rectangle {
        visible: isFinite(chart.markerX) && chart.points.length > 0
        x: visible ? chart.px(chart.markerX) : 0
        y: chart.padTop
        width: 2
        height: chart.plotH
        color: Theme.text
        opacity: 0.8
    }
}
