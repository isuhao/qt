import QtQuick 2.0
import Qt.labs.particles 2.0

Rectangle{
    color: "goldenrod"
    width: 800
    height: 800
    ParticleSystem{ id: sys }
    SpriteParticle{
        system: sys
        anchors.fill: parent
        sprites: [Sprite{
            name: "initial"
            source: "squarefacesprite.png"
            frames: 6
            duration: 0
            to: {"happy":0.2, "silly":0.2, "sad":0.2, "cyclops":0.1, "evil":0.1, "love":0.1, "boggled":0.1}
        }, Sprite{
            name: "silly"
            source: "squarefacesprite.png"
            frames: 6
            duration: 120
        }, Sprite{
            name: "happy"
            source: "squarefacesprite2.png"
            frames: 6
            duration: 120
        }, Sprite{
            name: "sad"
            source: "squarefacesprite3.png"
            frames: 6
            duration: 120
        }, Sprite{
            name: "cyclops"
            source: "squarefacesprite4.png"
            frames: 6
            duration: 120
        }, Sprite{
            name: "evil"
            source: "squarefacesprite5.png"
            frames: 6
            duration: 120
        }, Sprite{
            name: "love"
            source: "squarefacesprite6.png"
            frames: 6
            duration: 120
        }, Sprite{
            name: "boggled"
            source: "squarefacesprite7.png"
            frames: 6
            duration: 120
        }]
    }
    TrailEmitter{
        id: particleEmitter
        system: sys
        width: parent.width
        particlesPerSecond: 16
        particleDuration: 8000
        emitting: true
        speed: AngleVector{angle: 90; magnitude: 300; magnitudeVariation: 100; angleVariation: 5}
        acceleration: PointVector{ y: 10 }
        particleSize: 30
        particleSizeVariation: 10
    }
    Binding{
        target: particleEmitter
        property: "y"
        value: ma.mouseY
        when: ma.mouseX !=0 || ma.mouseY!=0
    }
    MouseArea{
        id: ma
        anchors.fill: parent
    }
}