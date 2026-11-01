// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import ChristmasReckoning

// Gentle procedural snowfall (shader, no particles). Freezes for reduced-motion users.
ShaderEffect {
    id: snow
    property vector2d viewSize: Qt.vector2d(width, height)
    property real time: 0
    property real density: 0.55

    fragmentShader: "qrc:/shaders/snow.frag.qsb"

    FrameAnimation {
        running: !Platform.reducedMotion && snow.visible
        onTriggered: snow.time += frameTime
    }
}
