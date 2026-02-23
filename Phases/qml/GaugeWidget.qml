import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    color: "#2d2d2d"
    radius: 10
    
    property string title: "Gauge"
    property double gaugeValue: 0     
    property color gaugeColor: "#00d4ff"  
    
    Column {
        anchors.centerIn: parent
        spacing: 10
        
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.title
            font.pixelSize: 16
            font.bold: true
            color: "#ffffff"
        }
        
        Item {
            width: 120
            height: 120
            
            Rectangle {
                anchors.centerIn: parent
                width: 120
                height: 120
                radius: 60
                color: "transparent"
                border.color: "#444444"
                border.width: 8
            }
            
            Canvas {
                id: canvas
                anchors.fill: parent
                
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    
                    var centerX = width / 2;
                    var centerY = height / 2;
                    var radius = 52;
                    
                    ctx.beginPath();
                    ctx.arc(centerX, centerY, radius, 
                           -Math.PI / 2, 
                           -Math.PI / 2 + (root.gaugeValue / 100) * 2 * Math.PI);
                    ctx.lineWidth = 8;
                    ctx.strokeStyle = root.gaugeColor;
                    ctx.stroke();
                }
                
                Connections {
                    target: root
                    function onGaugeValueChanged() {
                        canvas.requestPaint();
                    }
                }
            }
            
            Text {
                anchors.centerIn: parent
                text: root.gaugeValue.toFixed(1) + "%"
                font.pixelSize: 24
                font.bold: true
                color: root.gaugeColor
            }
        }
    }
}