import QtQuick

FocusScope {
    id: root
    focus: true

    // 2D Array defining rows and columns of inputs in your view:
    // fieldsGrid: [
    //     [input1, input2],
    //     [input3, input4, input5]
    // ]
    property var fieldsGrid: []
    
    // Optional default button to focus/trigger when Enter is pressed on the last field
    property Item submitButton: null

    // Optional field to focus initially on view load
    property Item initialFocusField: null

    // Signal emitted when Escape is pressed
    signal cancelRequested()

    function focusField(field) {
        if (!field) return
        if (typeof field.focusAndOpen === "function") {
            field.focusAndOpen()
        } else if (typeof field.focusAndSelect === "function") {
            field.focusAndSelect()
        } else if (typeof field.forceActiveFocus === "function") {
            field.forceActiveFocus()
        }
        if (typeof field.focusInput !== "undefined") {
            field.focusInput = true
        }
        if (typeof field.focus !== "undefined") {
            field.focus = true
        }
    }

    function findFieldCoords(field) {
        if (!fieldsGrid || !Array.isArray(fieldsGrid)) return null
        for (var r = 0; r < fieldsGrid.length; r++) {
            var row = fieldsGrid[r]
            if (!row || !Array.isArray(row)) continue
            for (var c = 0; c < row.length; c++) {
                if (row[c] === field) {
                    return { row: r, col: c }
                }
            }
        }
        return null
    }

    function handleNavigation(field, action) {
        var pos = findFieldCoords(field)
        if (!pos) return

        if (action === "next") {
            // Move to next column in current row, or first column in next row
            if (pos.col < fieldsGrid[pos.row].length - 1) {
                focusField(fieldsGrid[pos.row][pos.col + 1])
            } else if (pos.row < fieldsGrid.length - 1) {
                focusField(fieldsGrid[pos.row + 1][0])
            } else if (submitButton) {
                focusField(submitButton)
            }
        } else if (action === "prev" || action === "left") {
            if (pos.col > 0) {
                focusField(fieldsGrid[pos.row][pos.col - 1])
            } else if (pos.row > 0) {
                var prevRow = fieldsGrid[pos.row - 1]
                if (prevRow && prevRow.length > 0) {
                    focusField(prevRow[prevRow.length - 1])
                }
            }
        } else if (action === "right") {
            if (pos.col < fieldsGrid[pos.row].length - 1) {
                focusField(fieldsGrid[pos.row][pos.col + 1])
            }
        } else if (action === "up") {
            if (pos.row > 0) {
                var targetRow = fieldsGrid[pos.row - 1]
                if (targetRow && targetRow.length > 0) {
                    var targetCol = Math.min(pos.col, targetRow.length - 1)
                    focusField(targetRow[targetCol])
                }
            }
        } else if (action === "down") {
            if (pos.row < fieldsGrid.length - 1) {
                var targetRow2 = fieldsGrid[pos.row + 1]
                if (targetRow2 && targetRow2.length > 0) {
                    var targetCol2 = Math.min(pos.col, targetRow2.length - 1)
                    focusField(targetRow2[targetCol2])
                }
            } else if (submitButton) {
                focusField(submitButton)
            }
        }
    }

    function registerGrid() {
        if (!fieldsGrid || !Array.isArray(fieldsGrid)) return
        for (var r = 0; r < fieldsGrid.length; r++) {
            var row = fieldsGrid[r]
            if (!row || !Array.isArray(row)) continue
            for (var c = 0; c < row.length; c++) {
                var item = row[c]
                if (!item) continue

                (function(targetItem) {
                    try {
                        if (targetItem.returnPressed && typeof targetItem.returnPressed.connect === "function") {
                            targetItem.returnPressed.connect(function() { handleNavigation(targetItem, "next") })
                        }
                    } catch (e1) {}

                    try {
                        if (targetItem.leftPressed && typeof targetItem.leftPressed.connect === "function") {
                            targetItem.leftPressed.connect(function() { handleNavigation(targetItem, "left") })
                        }
                    } catch (e2) {}

                    try {
                        if (targetItem.rightPressed && typeof targetItem.rightPressed.connect === "function") {
                            targetItem.rightPressed.connect(function() { handleNavigation(targetItem, "right") })
                        }
                    } catch (e3) {}

                    try {
                        if (targetItem.upPressed && typeof targetItem.upPressed.connect === "function") {
                            targetItem.upPressed.connect(function() { handleNavigation(targetItem, "up") })
                        }
                    } catch (e4) {}

                    try {
                        if (targetItem.downPressed && typeof targetItem.downPressed.connect === "function") {
                            targetItem.downPressed.connect(function() { handleNavigation(targetItem, "down") })
                        }
                    } catch (e5) {}
                })(item)
            }
        }
    }

    Component.onCompleted: {
        Qt.callLater(function() {
            registerGrid()
            if (initialFocusField) {
                focusField(initialFocusField)
            } else if (fieldsGrid && fieldsGrid.length > 0 && fieldsGrid[0].length > 0) {
                focusField(fieldsGrid[0][0])
            }
        })
    }
}
