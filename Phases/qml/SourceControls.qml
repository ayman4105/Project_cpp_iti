import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    color: "#2d2d2d"
    radius: 5
    
    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15
        
        // Source toggles
        CheckBox {
            id: fileSourceCheck
            text: "File Source"
            checked: false
            onCheckedChanged: backend.setFileSourceEnabled(checked)
            
            contentItem: Text {
                text: fileSourceCheck.text
                font.pixelSize: 14
                color: "#ffffff"
                leftPadding: fileSourceCheck.indicator.width + 10
                verticalAlignment: Text.AlignVCenter
            }
        }
        
        CheckBox {
            id: socketSourceCheck
            text: "Socket Source"
            checked: false
            onCheckedChanged: backend.setSocketSourceEnabled(checked)
            
            contentItem: Text {
                text: socketSourceCheck.text
                font.pixelSize: 14
                color: "#ffffff"
                leftPadding: socketSourceCheck.indicator.width + 10
                verticalAlignment: Text.AlignVCenter
            }
        }
        
        CheckBox {
            id: someipSourceCheck
            text: "SOME/IP Source"
            checked: true
            onCheckedChanged: backend.setSomeIPSourceEnabled(checked)
            
            contentItem: Text {
                text: someipSourceCheck.text
                font.pixelSize: 14
                color: "#ffffff"
                leftPadding: someipSourceCheck.indicator.width + 10
                verticalAlignment: Text.AlignVCenter
            }
        }
        
        Item { Layout.fillWidth: true }
        
        // Control buttons
        Button {
            text: backend.isRunning ? "Stop" : "Start"
            onClicked: {
                if (backend.isRunning)
                    backend.stop()
                else
                    backend.start()
            }
            
            background: Rectangle {
                color: parent.pressed ? "#005577" : "#0088aa"
                radius: 5
            }
            
            contentItem: Text {
                text: parent.text
                font.pixelSize: 14
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
        
        Button {
            text: "Clear Logs"
            onClicked: backend.clearLogs()
            
            background: Rectangle {
                color: parent.pressed ? "#aa5500" : "#dd7700"
                radius: 5
            }
            
            contentItem: Text {
                text: parent.text
                font.pixelSize: 14
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}