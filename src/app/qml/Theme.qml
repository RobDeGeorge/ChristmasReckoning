// SPDX-License-Identifier: GPL-3.0-or-later
pragma Singleton
import QtQuick

// Night-sky palette built from the original site's brand colours
// (evergreen #183e35, ink #213f34, red #a7313b, gold #d7b678, paper #f8f6ef).
QtObject {
    // Surfaces
    readonly property color sky: "#050d0b"
    readonly property color skyHigh: "#0b2420"

    // Text
    readonly property color text: "#f4f0e4"
    readonly property color textMuted: "#b4c3b8"
    readonly property color textFaint: "#7f968a"

    // Accents
    readonly property color gold: "#d7b678"
    readonly property color goldBright: "#f1d39a"
    readonly property color red: "#e05a6a"
    readonly property color nogo: "#ff7272"
    readonly property color amber: "#f3b75a"

    // Type
    readonly property string sansFamily: "DM Sans"
    readonly property string displayFamily: "Playfair Display"
    readonly property string monoFamily: "JetBrains Mono"

    // Spacing scale
    readonly property int s1: 4
    readonly property int s2: 8
    readonly property int s3: 12
    readonly property int s4: 16
    readonly property int s5: 24
    readonly property int s6: 32
    readonly property int s7: 48
}
