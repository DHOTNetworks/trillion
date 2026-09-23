#pragma once

#include <QDialog>
#include <QString>
#include <QPushButton>
#include <QLabel>

namespace MahadevERP {

class PrintCopyOptionsDialog : public QDialog {
    Q_OBJECT

public:
    enum class Mode {
        Print,
        Pdf
    };

    explicit PrintCopyOptionsDialog(Mode mode, const QString& invoiceNo = "", const QString& customerName = "", QWidget* parent = nullptr);

    QString selectedCopyType() const { return m_selectedCopy; }

    static QString selectCopyType(Mode mode, const QString& invoiceNo, const QString& customerName, QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupUi();
    void chooseCopy(const QString& copyType);

    Mode m_mode = Mode::Print;
    QString m_invoiceNo;
    QString m_customerName;
    QString m_selectedCopy;
};

} // namespace MahadevERP
