// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import ChristmasReckoning

// Ray-traced Earth with a real day/night terminator and the delivery layer.
// Drag to rotate, scroll or pinch to zoom.
Item {
    id: root

    property real centerLatDeg: 22
    property real centerLonDeg: 10
    property real distance: 3.3          // camera distance in Earth radii
    property real fovDeg: 32
    property bool mapMode: false
    property real mapZoom: 1
    property double timeMs: Platform.nowMs()
    property bool interactive: true
    property real gridAlpha: 0.14
    property real offsetX: 0             // globe centre offset from the item centre (pixels)
    property real offsetY: 0
    property bool showDeliveries: false
    property real frontierZone: 0
    property real frontierRank: 0
    readonly property bool texturesReady: dayImage.status === Image.Ready
                                          && nightImage.status === Image.Ready
    signal userInteracted()

    // Screen position of a geographic point: {x, y, visible}. Mirrors the shader's camera exactly.
    function project(latDeg, lonDeg) {
        const lat = latDeg * Math.PI / 180, lon = lonDeg * Math.PI / 180;
        if (mapMode) {
            const mapWidth = Math.min(width, 2 * height) * mapZoom;
            const pxPerRad = mapWidth / (2 * Math.PI);
            let dLon = lon - centerLonDeg * Math.PI / 180;
            dLon = ((dLon + Math.PI) % (2 * Math.PI) + 2 * Math.PI) % (2 * Math.PI) - Math.PI;
            return { x: width / 2 + offsetX + dLon * pxPerRad, y: height / 2 + offsetY - lat * pxPerRad, visible: true };
        }
        const p = [Math.cos(lat) * Math.cos(lon), Math.cos(lat) * Math.sin(lon), Math.sin(lat)];
        const cLat = centerLatDeg * Math.PI / 180, cLon = centerLonDeg * Math.PI / 180;
        const f = [Math.cos(cLat) * Math.cos(cLon), Math.cos(cLat) * Math.sin(cLon), Math.sin(cLat)];
        const e = [-Math.sin(cLon), Math.cos(cLon), 0];
        const n = [-Math.sin(cLat) * Math.cos(cLon), -Math.sin(cLat) * Math.sin(cLon), Math.cos(cLat)];
        const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
        const qx = dot(p, e), qy = dot(p, n), qz = dot(p, f);
        const t = Math.tan(fovDeg * Math.PI / 360);
        const depth = distance - qz;
        return {
            x: width / 2 + offsetX + qx / (depth * t) * height / 2,
            y: height / 2 + offsetY - qy / (depth * t) * height / 2,
            visible: qz > 1 / distance
        };
    }

    // Radius of the globe's disc on screen, in pixels.
    readonly property real discRadius: {
        const t = Math.tan(fovDeg * Math.PI / 360);
        const sinA = 1 / distance;
        return Math.tan(Math.asin(sinA)) / t * height / 2;
    }

    Image {
        id: dayImage
        visible: false
        source: Platform.assetBase + "textures/earth-day.jpg"
        asynchronous: true
        mipmap: true
        cache: true
    }

    Image {
        id: nightImage
        visible: false
        source: Platform.assetBase + "textures/earth-night.jpg"
        asynchronous: true
        mipmap: true
        cache: true
    }

    Image {
        id: deliveryImage
        visible: false
        source: "qrc:/data/delivery.png"
        smooth: false
        mipmap: false
    }

    ShaderEffect {
        id: effect
        anchors.fill: parent

        property variant dayTex: dayImage
        property variant nightTex: nightImage
        property variant deliveryTex: deliveryImage
        property vector2d viewSize: Qt.vector2d(width, height)
        property vector2d viewOffset: Qt.vector2d(root.offsetX, root.offsetY)
        property real centerLat: root.centerLatDeg * Math.PI / 180
        property real centerLon: root.centerLonDeg * Math.PI / 180
        property real camDistance: root.distance
        property real fovY: root.fovDeg * Math.PI / 180
        property vector3d sunDir: Astro.sunDirection(root.timeMs)
        property real mapMode: root.mapMode ? 1 : 0
        property real mapZoom: root.mapZoom
        property real texturesReady: root.texturesReady ? 1 : 0
        property real gridAlpha: root.gridAlpha
        property real deliveryAlpha: root.showDeliveries ? 1 : 0
        property real frontierZone: root.frontierZone
        property real frontierRank: root.frontierRank
        property real pulse: 0

        fragmentShader: "qrc:/shaders/globe.frag.qsb"

        Behavior on texturesReady { NumberAnimation { duration: 600 } }
        Behavior on deliveryAlpha { NumberAnimation { duration: 400 } }

        SequentialAnimation on pulse {
            running: root.showDeliveries && !Platform.reducedMotion
            loops: Animation.Infinite
            NumberAnimation { to: 1; duration: 700; easing.type: Easing.InOutSine }
            NumberAnimation { to: 0; duration: 700; easing.type: Easing.InOutSine }
        }
    }

    DragHandler {
        id: drag
        enabled: root.interactive
        target: null
        property real startLat
        property real startLon
        onActiveChanged: {
            if (active) {
                startLat = root.centerLatDeg;
                startLon = root.centerLonDeg;
                root.userInteracted();
            }
        }
        onTranslationChanged: {
            if (!active)
                return;
            if (root.mapMode) {
                const pxPerDeg = Math.min(root.width, 2 * root.height) * root.mapZoom / 360;
                root.centerLonDeg = startLon - translation.x / pxPerDeg;
                return;
            }
            const degPerPx = 70 / root.height * (root.distance - 1);
            root.centerLonDeg = startLon - translation.x * degPerPx;
            root.centerLatDeg = Math.max(-85, Math.min(85, startLat + translation.y * degPerPx));
        }
    }

    WheelHandler {
        enabled: root.interactive
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: event => {
            const factor = Math.pow(1.0015, -event.angleDelta.y);
            if (root.mapMode)
                root.mapZoom = Math.max(1, Math.min(6, root.mapZoom / factor));
            else
                root.distance = Math.max(1.35, Math.min(8, 1 + (root.distance - 1) * factor));
            root.userInteracted();
        }
    }

    PinchHandler {
        enabled: root.interactive
        target: null
        property real startDistance
        property real startZoom
        onActiveChanged: {
            if (active) {
                startDistance = root.distance;
                startZoom = root.mapZoom;
                root.userInteracted();
            }
        }
        onActiveScaleChanged: {
            if (root.mapMode)
                root.mapZoom = Math.max(1, Math.min(6, startZoom * activeScale));
            else
                root.distance = Math.max(1.35, Math.min(8, 1 + (startDistance - 1) / activeScale));
        }
    }
}
