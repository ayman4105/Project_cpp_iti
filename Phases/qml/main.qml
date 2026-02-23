import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1200
    height: 800
    title: "Telemetry Logging System - Dashboard"
    
    color: "#1e1e1e"  // Dark background
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        // ═══════════════════════════════════════════════════════════
        // HEADER
        // ═══════════════════════════════════════════════════════════
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#2d2d2d"
            radius: 5
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                
                Text {
                    text: "Telemetry Logging System"
                    font.pixelSize: 24
                    font.bold: true
                    color: "#00d4ff"
                }
                
                Item { Layout.fillWidth: true }
                
                Rectangle {
                    width: 15
                    height: 15
                    radius: 7.5
                    color: backend.isRunning ? "#00ff00" : "#ff0000"
                }
                
                Text {
                    text: backend.isRunning ? "Running" : "Stopped"
                    font.pixelSize: 16
                    color: "#ffffff"
                }
            }
        }
        
        // ═══════════════════════════════════════════════════════════
        // GAUGES ROW
        // ═══════════════════════════════════════════════════════════
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            spacing: 10
            
            GaugeWidget {
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: "CPU Usage"
                gaugeValue: backend.cpuUsage     
                gaugeColor: "#ff6b6b" 
            }
            
            GaugeWidget {
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: "RAM Usage"
                gaugeValue: backend.ramUsage   
                gaugeColor: "#4ecdc4"  
            }
            
            GaugeWidget {
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: "GPU Usage"
                gaugeValue: backend.gpuUsage   
                gaugeColor: "#95e1d3"
            }
        }
        
        // ═══════════════════════════════════════════════════════════
        // LOG VIEWER
        // ═══════════════════════════════════════════════════════════
        LogViewer {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        
        // ═══════════════════════════════════════════════════════════
        // SOURCE CONTROLS
        // ═══════════════════════════════════════════════════════════
        SourceControls {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
        }
        
        // ═══════════════════════════════════════════════════════════
        // STATUS BAR
        // ═══════════════════════════════════════════════════════════
        StatusBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
        }
    }
}