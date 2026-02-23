import QtQuick 2.15
import QtQuick.Layouts 1.15

Rectangle {
    color: "#1e1e1e"
    
    RowLayout {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 20
        
        Text {
            text: "Status: " + (backend.isRunning ? "Running" : "Stopped")
            font.pixelSize: 12
            color: backend.isRunning ? "#00ff00" : "#ff0000"
        }
        
        Rectangle {
            width: 1
            height: 20
            color: "#444444"
        }
        
        Text {
            text: "Messages: " + backend.messageCount
            font.pixelSize: 12
            color: "#ffffff"
        }
        
        Rectangle {
            width: 1
            height: 20
            color: "#444444"
        }
        
        Text {
            text: "Buffer: " + backend.bufferUsage + "/200"
            font.pixelSize: 12
            color: "#ffffff"
        }
        
        Item { Layout.fillWidth: true }
        
        Text {
            text: Qt.formatDateTime(new Date(), "yyyy-MM-dd hh:mm:ss")
            font.pixelSize: 12
            color: "#888888"
            
            Timer {
                interval: 1000
                running: true
                repeat: true
                onTriggered: parent.text = Qt.formatDateTime(new Date(), "yyyy-MM-dd hh:mm:ss")
            }
        }
    }
}