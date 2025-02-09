import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtQml.Models 2.15
import QtQuick.Dialogs 1.1

Window {
    id: root

    property int boardSize: boardModel ? boardModel.size() : 0

    visible: true
    width: 500
    height: 550
    title: "Color Lines"

    QtObject {
        id: internal

        property int grabbedIndex: -1
        property point grabbedBall: Qt.point(0,0)

        function getColor(colorRole) {
            return colorRole == 1 ? "red" : colorRole == 2 ? "blue" : colorRole == 3 ? "green" : colorRole == 4 ? "yellow" : "transparent";
        }

        function startBallMovingAnimation(fromRow, fromCol, toRow, toCol, colorRole) {
            // Convert board coordinates to pixel coordinates
            let cellSize = 50;
            let startX = fromCol * cellSize + tableView.x + 0.1*cellSize;
            let startY = fromRow * cellSize + tableView.y + 0.1*cellSize;
            let endX = toCol * cellSize + tableView.x + 0.1*cellSize;
            let endY = toRow * cellSize + tableView.y + 0.1*cellSize;

            console.debug("start animation: " + startX + ", " + startY + ", colorRole: " + colorRole);

            floatingBall.x = startX;
            floatingBall.y = startY;
            floatingBall.visible = true;
            floatingBall.colorRole = colorRole;
            floatingBall.toRow = toRow;
            floatingBall.toCol = toCol;

            // Start animation
            moveAnimX.to = endX;
            moveAnimY.to = endY;
            moveAnimX.start();
            moveAnimY.start();
        }
    }

    Connections {
        target: boardModel
        function onGameOver() {
            messageDialog.open()
        }
    }

    MessageDialog {
        id: messageDialog

        title: "Game is over"
        text: boardModel.score > 0 ? ("You scored " + boardModel.score) : "You loose!"
        icon: MessageDialog.Information
        standardButtons: StandardButton.Ok

        onAccepted: {
            boardModel.clearBoard();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        TableView {
            id: tableView

            Layout.alignment: Qt.AlignCenter
            width: 450
            height: 450
            model: boardModel
            delegate: Rectangle {
                implicitWidth: 50
                implicitHeight: 50
                border.color: "black"
                border.width: internal.grabbedIndex === index ? 3 : 1
                color: /*internal.grabbedIndex === index ? "lightgrey" :*/ "white"

                Rectangle {
                    id: ball

                    width: parent.width * 0.8
                    height: parent.height * 0.8
                    radius: width / 2
                    anchors.centerIn: parent
                    color: internal.getColor(colorRole)
                    Behavior on color { ColorAnimation { duration: isMoved ? 0 : 400; } }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            var row = index%root.boardSize;
                            var col = Math.floor(index/root.boardSize);

                            console.debug("Clicked " + index + " " + row + "," + col + ", grabbedIndex " + internal.grabbedIndex)

                            if (internal.grabbedIndex !== -1) {
                                var colorValue = boardModel.startBallMoving(internal.grabbedBall.x, internal.grabbedBall.y, row, col);
                                if (colorValue > 0) {
                                    internal.startBallMovingAnimation(internal.grabbedBall.x, internal.grabbedBall.y, row, col, colorValue)
                                }
                                internal.grabbedIndex = -1;
                            }
                            else {
                                internal.grabbedBall = Qt.point(row, col);
                                internal.grabbedIndex = index;
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 20

            Button {
                text: "New Game"
                onClicked: boardModel.newGame()
            }

            Text {
                text: "Score: " + boardModel.score
                font.pixelSize: 20
            }
        }
    }

    //Floating ball overlay object for smooth movement
    Rectangle {
        id: floatingBall

        property int colorRole
        property int toRow: 0
        property int toCol: 0

        visible: false
        width: 40
        height: 40
        radius: width / 2
        color: internal.getColor(colorRole)

        PropertyAnimation {
            id: moveAnimX

            target: floatingBall
            property: "x"
            duration: 600
            easing.type: Easing.InOutQuad
        }

        PropertyAnimation {
            id: moveAnimY

            target: floatingBall
            property: "y"
            duration: 600
            easing.type: Easing.InOutQuad
            onFinished: {
                boardModel.finishBallMoving(floatingBall.toRow, floatingBall.toCol, floatingBall.colorRole);
                floatingBall.visible = false;
            }
        }
    }
}
