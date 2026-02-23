import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    color: "#2d2d2d"
    radius: 5
    
    Column {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 5
        
        // Header
        Text {
            text: "Realtime Log Viewer"
            font.pixelSize: 18
            font.bold: true
            color: "#00d4ff"
        }
        
        // Log list
        ListView {
            width: parent.width
            height: parent.height - 30
            clip: true
            
            model: logModel
            
            delegate: Rectangle {
                width: ListView.view.width
                height: logText.height + 10
                color: index % 2 == 0 ? "#1e1e1e" : "#252525"
                
                Text {
                    id: logText
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 5
                    anchors.verticalCenter: parent.verticalCenter
                    
                    text: formattedText
                    font.family: "Courier New"
                    font.pixelSize: 12
                    color: getColorForLevel(level)
                    wrapMode: Text.NoWrap
                }
                
                function getColorForLevel(level) {
                    switch(level) {
                        case "ERROR": return "#ff6b6b";
                        case "WARNING": return "#ffd93d";
                        case "INFO": return "#6bcf7f";
                        case "DEBUG": return "#95e1d3";
                        default: return "#ffffff";
                    }
                }
            }
            
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AlwaysOn
            }
            
            // Auto-scroll to bottom
            onCountChanged: {
                positionViewAtEnd()
            }
        }
    }
}