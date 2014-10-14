import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.1

Item {
    id: metaInfoItem

    width: parent.width
    height: metaNameField.height + defaultMargins

    property alias metaNameField: metaNameField
    property alias metaValueField: metaValueField
    property alias deleteMetaButton: deleteMetaButton

    RowLayout {
        anchors.fill: parent
        anchors.margins: defaultMargins

        TextField {
            id: metaNameField
            Layout.fillWidth: isArray ? false : true
            placeholderText : qsTr("Meta Key Name...")
            text: metaName
        }
        TextField {
            id: metaValueField
            Layout.fillWidth: true
            text: metaValue
        }
        Button {
            id:deleteMetaButton

            implicitHeight: metaNameField.height
            implicitWidth: implicitHeight

            style: ButtonStyle {
                background: Image {
                    anchors.centerIn: parent
                    source: "icons/application-exit.png"
                }
            }

            onClicked: {
                qmlMetaKeyModel.remove(index)// remove the visual item

                if(isArray){
                    for(var i = 0; i < qmlMetaKeyModel.count; i++){
                        qmlMetaKeyModel.set(i, {"metaName": "#" + i})
                    }

                }

                isEdited = true
            }

        }

    }

}

