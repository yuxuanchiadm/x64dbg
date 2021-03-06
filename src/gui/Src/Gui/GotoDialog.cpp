#include "GotoDialog.h"
#include "ui_GotoDialog.h"

GotoDialog::GotoDialog(QWidget* parent, bool allowInvalidExpression)
    : QDialog(parent),
      ui(new Ui::GotoDialog),
      allowInvalidExpression(allowInvalidExpression)
{
    //setup UI first
    ui->setupUi(this);
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    //initialize stuff
    if(!DbgIsDebugging()) //not debugging
        ui->labelError->setText(tr("<font color='red'><b>Not debugging...</b></font>"));
    else
        ui->labelError->setText(tr("<font color='red'><b>Invalid expression...</b></font>"));
    setOkEnabled(false);
    ui->editExpression->setFocus();
    validRangeStart = 0;
    validRangeEnd = 0;
    fileOffset = false;
    mValidateThread = new ValidateExpressionThread(this);
    mValidateThread->setOnExpressionChangedCallback(std::bind(&GotoDialog::validateExpression, this, std::placeholders::_1));

    connect(mValidateThread, SIGNAL(expressionChanged(bool, bool, dsint)), this, SLOT(expressionChanged(bool, bool, dsint)));
    connect(ui->editExpression, SIGNAL(textChanged(QString)), mValidateThread, SLOT(textChanged(QString)));
    connect(this, SIGNAL(finished(int)), this, SLOT(finishedSlot(int)));
}

GotoDialog::~GotoDialog()
{
    delete ui;
}

void GotoDialog::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    mValidateThread->start();
}

void GotoDialog::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    mValidateThread->stop();
    mValidateThread->wait();
}

void GotoDialog::validateExpression(QString expression)
{
    duint value;
    bool validExpression = DbgFunctions()->ValFromString(expression.toUtf8().constData(), &value);
    bool validPointer = validExpression && DbgMemIsValidReadPtr(value);
    this->mValidateThread->emitExpressionChanged(validExpression, validPointer, value);
}

void GotoDialog::setInitialExpression(const QString & expression)
{
    ui->editExpression->setText(expression);
    validateExpression(expression);
}

void GotoDialog::expressionChanged(bool validExpression, bool validPointer, dsint value)
{
    QString expression = ui->editExpression->text();
    if(!expression.length())
    {
        ui->labelError->setText(tr("<font color='red'><b>Empty expression...</b></font>"));
        setOkEnabled(false);
        expressionText.clear();
    }
    if(expressionText == expression)
        return;
    if(!DbgIsDebugging()) //not debugging
    {
        ui->labelError->setText(tr("<font color='red'><b>Not debugging...</b></font>"));
        setOkEnabled(false);
        expressionText.clear();
    }
    else if(!validExpression) //invalid expression
    {
        ui->labelError->setText(tr("<font color='red'><b>Invalid expression...</b></font>"));
        setOkEnabled(false);
        expressionText.clear();
    }
    else if(fileOffset)
    {
        duint offset = value;
        duint va = DbgFunctions()->FileOffsetToVa(modName.toUtf8().constData(), offset);
        QString addrText = QString(" %1").arg(va, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        if(va)
        {
            ui->labelError->setText(tr("<font color='#00DD00'><b>Correct expression! -&gt; </b></font>") + addrText);
            setOkEnabled(true);
            expressionText = expression;
        }
        else
        {
            ui->labelError->setText(tr("<font color='red'><b>Invalid file offset...</b></font>") + addrText);
            setOkEnabled(false);
            expressionText.clear();
        }
    }
    else
    {
        duint addr = value;
        QString addrText = QString(" %1").arg(addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        if(!validPointer)
        {
            ui->labelError->setText(tr("<font color='red'><b>Invalid memory address...</b></font>") + addrText);
            setOkEnabled(false);
            expressionText.clear();
        }
        else if(!IsValidMemoryRange(addr))
        {
            ui->labelError->setText(tr("<font color='red'><b>Memory out of range...</b></font>") + addrText);
            setOkEnabled(false);
            expressionText.clear();
        }
        else
        {
            char module[MAX_MODULE_SIZE] = "";
            char label[MAX_LABEL_SIZE] = "";
            if(DbgGetLabelAt(addr, SEG_DEFAULT, label)) //has label
            {
                if(DbgGetModuleAt(addr, module) && !QString(label).startsWith("JMP.&"))
                    addrText = QString(module) + "." + QString(label);
                else
                    addrText = QString(label);
            }
            else if(DbgGetModuleAt(addr, module) && !QString(label).startsWith("JMP.&"))
                addrText = QString(module) + "." + QString("%1").arg(addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
            else
                addrText = QString("%1").arg(addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
            ui->labelError->setText(tr("<font color='#00DD00'><b>Correct expression! -&gt; </b></font>") + addrText);
            setOkEnabled(true);
            expressionText = expression;
        }
    }
}

bool GotoDialog::IsValidMemoryRange(duint addr)
{
    return ((!validRangeStart && !validRangeEnd) || (addr >= validRangeStart && addr < validRangeEnd));
}

void GotoDialog::setOkEnabled(bool enabled)
{
    ui->buttonOk->setEnabled(enabled || allowInvalidExpression);
}

void GotoDialog::on_buttonOk_clicked()
{
    QString expression = ui->editExpression->text();
    ui->editExpression->addLineToHistory(expression);
    ui->editExpression->setText("");
    expressionChanged(false, false, 0);
    expressionText = expression;
}

void GotoDialog::finishedSlot(int result)
{
    if(result == QDialog::Rejected)
        ui->editExpression->setText("");
    ui->editExpression->setFocus();
}
