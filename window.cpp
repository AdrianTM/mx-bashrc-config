#include <iostream>
#include <QSettings>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QFile>
#include <QDialog>
#include <QTextStream>
#include "window.h"
#include "ui_window.h"

Window::Window(QWidget *parent) :
    QWidget(parent), ui(new Ui::Window)
{
    DEBUG_MSG << "+++ Window::Window +++";
    ui->setupUi(this); //setting up UI
    setWindowTitle(APP_NAME+VERSION); //MACROS found in config.h
    connectAll(); //connects slots to signals
    otherSetup(); //setups widgets
    DEBUG_MSG << "--- Window::Window ---";
}

Window::~Window()
{
    DEBUG_MSG << "+++ Window::~Window +++";
    delete ui;
    DEBUG_MSG << "--- Window::~Window ---";
}

Window::Aliases Window::getAliases()
{
    DEBUG_MSG << "+++ Window::getAliases +++";
    Aliases rtn;
    QFile file(BASHRC);
    if(!file.open(QFile::Text | QFile::ReadOnly))
    {
        return rtn;
    }
    QTextStream stream(&file);
    QString tmp;
    while(stream.readLineInto(&tmp))
    {
        std::string scan = tmp.toStdString();
        if(scan.find('#') != std::string::npos)
            scan = scan.substr(0, scan.find("#"));
        size_t aliasKeywordBegin = scan.find("alias");
        if(aliasKeywordBegin == std::string::npos) continue;
        size_t aliasWordBegin = scan.find_first_not_of(' ', aliasKeywordBegin+5);
        if(aliasWordBegin == std::string::npos) continue;
        size_t equalSignPos = scan.find('=', aliasWordBegin);
        if(equalSignPos == std::string::npos) continue;
        QString word = scan.substr(aliasWordBegin, equalSignPos-aliasWordBegin).c_str();
        size_t endQuote = scan.rfind('\'');
        if(endQuote == std::string::npos) continue;
        QString command = scan.substr(equalSignPos+2, endQuote-equalSignPos-2).c_str();
        QPair<QString, QString> alias;
        alias.first = word;
        alias.second = command;
        rtn << alias;
    }
    DEBUG_MSG << "--- Window::getAliases ---";
    return rtn;
}

Window::Aliases Window::getAliasesFromTable()
{
    DEBUG_MSG << "+++ Window::getAliasesFromTable +++";
    Aliases rtn;
    for(int i = 0; i < ui->tableWidget->rowCount(); i++)
    {
        if(ui->tableWidget->item(i, 0)->text().isEmpty() ||
                ui->tableWidget->item(i, 1)->text().isEmpty())
            continue;
        QPair<QString, QString> alias;
        alias.first = ui->tableWidget->item(i, 0)->text();
        alias.second = ui->tableWidget->item(i, 1)->text();
        rtn << alias;
    }
    DEBUG_MSG << "--- Window::getAliases ---";
    return rtn;
}

void Window::setupConfig()
{
    DEBUG_MSG << "+++ Window::setupConfig +++";
    QString content = getBashrc();
    size_t begin = content.toStdString().find("#MXBASHRC_BEGIN");
    size_t end = content.toStdString().find("#MXBASHRC_END", begin+QString("#MXBASHRC_BEGIN").length());
    if(begin == std::string::npos || end == std::string::npos)
    {
        DEBUG_MSG << "+++ Window::setupConfig: Failed to parse file +++";
        return;
    }
    content = QString::fromStdString(content.toStdString().substr(begin, end+QString("MXBASHRC_END").length()));
    if(content.toStdString().find("color_prompt=no") != std::string::npos)
        ui->checkBox_NoColor->setChecked(true);
    //Check to see if the phrase 'color_prompt=yes' occurs three times
    else if(content.toStdString().find("color_prompt=yes",
                                       content.toStdString().find("color_prompt=yes",
                                       content.toStdString().find("color_prompt=yes")+1)+1) != std::string::npos &&
            content.toStdString().find("color_prompt=yes", content.toStdString().find("color_prompt=yes")+1) != std::string::npos &&
            content.toStdString().find("color_prompt=yes")+1 != std::string::npos+1)
        ui->checkBox_ForceColor->setChecked(true);
    else
        ui->checkBox_Default->setChecked(true);
    if(content.toStdString().find("PS1=\"$nocolor\\u@\\h: \\w \\n\\$") != std::string::npos)
        ui->checkBox_Newline->setChecked(true);
    if(content.toStdString().find("debian_chroot") != std::string::npos)
        ui->checkBox_DebianChroot->setChecked(true);
    if(content.toStdString().find("HISTCONTROL=$HISTCONTROL${HISTCONTROL+:}ignorespace") != std::string::npos)
        ui->checkBox_ignorespace->setChecked(true);

    DEBUG_MSG << "--- Window::setupConfig ---";
}

void Window::connectAll()
{
    DEBUG_MSG << "+++ Window::connectAll +++";
    connect(ui->pushButton, &QPushButton::clicked, this, qOverload<>(&Window::apply));
    connect(ui->pushButton_minus, &QPushButton::clicked, this, &Window::onAliasButtonRemove);
    connect(ui->pushButton_plus, &QPushButton::clicked, this, &Window::onAliasButtonAdd);
    connect(ui->pushButton_Restore, &QPushButton::clicked, this, &Window::restore);
    connect(ui->pushButton_About, &QPushButton::clicked, this, &Window::about);
    connect(ui->pushButton_Help, &QPushButton::clicked, this, &Window::help);
    DEBUG_MSG << "--- Window::connectAll ---";
}

void Window::setupPromptTokens()
{

}

void Window::otherSetup()
{
    DEBUG_MSG << "+++ Window::otherSetup +++";
    ui->tableWidget->setSelectionBehavior(QTableWidget::SelectRows); //Makes it so the user can only select rows
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true); //stretch the command column to the end
    //Code to add current aliases to the tableWidget
    Aliases list = getAliases();
    ui->tableWidget->clearContents(); ui->tableWidget->setColumnCount(2);
    QStringList headerText =
    {
        "Keyword",
        "Command"
    };
    ui->tableWidget->setHorizontalHeaderLabels(headerText);
    for(int i = 0; i < list.size(); i++)
    {
        ui->tableWidget->setRowCount(ui->tableWidget->rowCount()+1);
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(list.at(i).first));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(list.at(i).second));
    }
    setupConfig();
    QSettings settings("MX-Linux", "mx-bashrc");
    restoreGeometry(settings.value("geometery").toByteArray());
    DEBUG_MSG << "--- Window::otherSetup ---";
}

QList<int> Window::getAliasLines()
{
    DEBUG_MSG << "+++ Window::getAliasLines +++";
    QList<int> rtn;
    QFile file(BASHRC);
    if(!file.open(QFile::Text | QFile::ReadOnly)) return rtn;
    QTextStream stream(&file);
    QString tmp;
    int loop = 0;
    while(stream.readLineInto(&tmp))
    {
        loop++;
        std::string scan = tmp.toStdString();
        if(scan.find('#') != std::string::npos)
            scan = scan.substr(0, scan.find("#"));
        size_t aliasKeywordBegin = scan.find("alias");
        if(aliasKeywordBegin == std::string::npos) continue;
        rtn << loop;
    }
    return rtn;
    DEBUG_MSG << "--- Window::getAliasLines ---";
}

QList<PromptToken> Window::getPromptTokens()
{
    DEBUG_MSG << "+++ Window::getPromptTokens +++";
    DEBUG_MSG << "--- Window::getPromptTokens ---";
}

QStringList Window::getBashrcList(int* sucess)
{
    DEBUG_MSG << "+++ QStringList Window::getBashrc +++";
    QFile file(BASHRC);
    QStringList rtn;
    if(!file.open(QFile::Text | QFile::ReadWrite))
    {
        if(sucess != nullptr)
            *sucess = 1;
        return rtn;
    }
    else
    {
        if(sucess != nullptr)
            *sucess = 0;
    }
    QTextStream stream(&file);
    QString tmp;
    while(stream.readLineInto(&tmp)) rtn += tmp;
    file.close();
    DEBUG_MSG << "--- QStringList Window::getBashrc ---";
    return rtn;
}
QString Window::getBashrc(int* sucess)
{
    DEBUG_MSG << "+++ QString Window::getBashrc +++";
    int rv;
    QStringList data = getBashrcList(&rv);
    if(sucess != nullptr)
    {
        *sucess = rv;
    }
    QString rtn;
    foreach(QString i, data)
    {
        rtn += i + "\n";
    }
    DEBUG_MSG << "--- QString Window::getBashrc ---";
    return rtn;
}

QString Window::getBashrcBetweenStartAndEnd(int *sucess)
{
    DEBUG_MSG << "+++ QString Window::getBashrcBetweenStartAndEnd +++";
    if(sucess != nullptr) *sucess = 0;
    QString bashrc = getBashrc();
    size_t start = bashrc.toStdString().find("#MXBASHRC_BEGIN");
    size_t end = bashrc.toStdString().find("#MXBASHRC_END");
    if(end <= start) end = bashrc.toStdString().find("#MXBASHRC_BEGIN", end+1);
    if(start == std::string::npos || end == std::string::npos) if(sucess != nullptr) *sucess = -1;
    end += QString("#MXBASHRC_END").length();
    QString rtn = QString(bashrc.toStdString().substr(start, end-start).c_str());
    DEBUG_MSG << "--- QString Window::getBashrcBetweenStartAndEnd ---";
    return rtn;
}

void Window::closeEvent(QCloseEvent *)
{
    DEBUG_MSG << "+++ Window::closeEvent +++";
    QSettings settings("MX-Linux", "mx-bashrc");
    settings.setValue("geometery", saveGeometry());
    DEBUG_MSG << "--- Window::closeEvent ---";
}

void Window::help()
{
    DEBUG_MSG << "+++ Window::help +++";
    QMessageBox::about(this, APP_NAME+ "-"+VERSION, "Information About Options: Hover over it and a tool tip will appear\nThe Reformat button makes it possible to apply changes, and Restore restores the bashrc to the MX Linux default");
    DEBUG_MSG << "--- Window::help ---";
}

void Window::about()
{
    DEBUG_MSG << "+++ Window::about +++";
    QMessageBox::about(this, APP_NAME+" - "+VERSION, "A program designed to edit your ~/.bashrc easily");
    DEBUG_MSG << "--- Window::about ---";
}

void Window::reformat()
{
    DEBUG_MSG << "+++ Window::reformat +++";
    int user = QMessageBox::warning(this, "Warning - " + APP_NAME, "This option will reformat ~/.bashrc, it may erase some of your configuration, and so ~/.bashrc is being backed up to ~/.bashrc.bkup",
                                    QMessageBox::Ok | QMessageBox::Cancel);
    if(user == QMessageBox::Cancel) return;
    QString orgBashrc = getBashrc();
    QFile bkupFile(QDir::homePath() + "/.bashrc.bkup");
    if(!bkupFile.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox::warning(this, "Warning - " + APP_NAME, "Operation failed to open ~/.bashrc.bkup for copying. Aborted.", QMessageBox::Ok);
        return;
    }
    QTextStream bkupStream(&bkupFile);
    bkupStream << orgBashrc;
    bkupFile.close();

    fileObj.resize(0);

    fileStream << "#MXBASHRC_BEGIN\n#MXBASHRC_END\n";

    fileObj.close();

    apply();

    DEBUG_MSG << "--- Window::reformat ---";
}

void Window::apply()
{
    DEBUG_MSG << "+++ Window::apply +++";
    QString tmp;
    QString content;
    QStringList list = getBashrc();

    //get rid of alias lines
    QList<int> lines = getAliasLines();
    for(int i = 0; i < list.size();i++)
    {
        bool add = true;
        for(int j = 0; j < lines.size(); j++)
        {
            if(lines.at(j) == i+1) add = false;
        }
        if(add) content.append(list.at(i) + "\n");
    }

    //Find the comments for the beginning and end, and remove it
    size_t begin = content.toStdString().find("#MXBASHRC_BEGIN");
    size_t end = content.toStdString().find("#MXBASHRC_END");
    while(end <= begin)
    {
        end = content.toStdString().find("#MXBASHRC_END", begin);
        if(end == std::string::npos) break;
    }
    end += QString("#MXBASHRC_END").length();
    if(begin == std::string::npos || end == std::string::npos)
    {
        int u = QMessageBox::warning(this, "Warning - " + BASHRC, "Was unable to parse file, do you wish to reformat the bashrc so", QMessageBox::Ok | QMessageBox::No);
        if(u == QMessageBox::Ok) reformat();
        //reformat applies at the end so returning stops it from repeating
        return;
    }
    content.remove(begin, end-begin);
    QStringList newCode;
#define LINE(msg) newCode << QString(msg)
    Aliases aliases;
    aliases = getAliasesFromTable();
    typedef QPair<QString, QString> Alias;
    foreach( Alias i, aliases)
    {
        LINE(QString("alias " + i.first + "='" + i.second +"'").toStdString().c_str());
    }
    LINE("#MXBASHRC_BEGIN");
    LINE("[ -z \"$PS1\" ] && return");
    if(ui->checkBox_ignorespace->isChecked())
        LINE("HISTCONTROL=$HISTCONTROL${HISTCONTROL+:}ignorespace");

    else
        LINE("HISTCONTROL=ignoreboth");

    LINE("shopt -s histappend");
    LINE("shopt -s checkwinsize");
    LINE("red='\\[\\e[0;31m\\]'");
    LINE("RED='\\[\\e[1;31m\\]'");
    LINE("blue='\\[\\e[0;34m\\]'");
    LINE("BLUE='\\[\\e[1;34m\\]'");
    LINE("cyan='\\[\\e[0;36m\\]'");
    LINE("CYAN='\\[\\e[1;36m\\]'");
    LINE("green='\\[\\e[0;32m\\]'");
    LINE("GREEN='\\[\\e[1;32m\\]'");
    LINE("yellow='\\[\\e[0;33m\\]'");
    LINE("YELLOW='\\[\\e[1;33m\\]'");
    LINE("purple='\\[\\e[0;35m\\]'");
    LINE("PURPLE='\\[\\e[1;35m\\]'");
    LINE("nocolor='\\[\\e[00m\\]'");

    LINE("case \"$TERM\" in");
    LINE("  xterm-color) color_prompt=yes;;");
    LINE("  xterm-256color) color_prompt=yes;;");
    LINE("  xterm) color_prompt=yes;;");
    LINE("esac");
    if(ui->checkBox_ForceColor->isChecked())
        LINE("color_prompt=yes");
    else if(ui->checkBox_NoColor->isChecked())
        LINE("color_prompt=no");
    LINE("if [ \"$color_prompt\" = yes ]; then");
    QString prompt("");
    prompt.append("   PS1=\"$nocolor$PURPLE\\u$nocolor@$CYAN\\h$nocolor: $GREEN\\w$nocolor ");
    if(ui->checkBox_Newline->isChecked())
        prompt.append("\\n");
    prompt.append("\\$ \"");
    LINE(prompt);
    LINE("else");
    prompt = "";
    prompt.append("   PS1=\"$nocolor\\u@\\h: \\w ");
    if(ui->checkBox_Newline->isChecked())
        prompt.append("\\n");
    prompt.append("\\$ \"");
    LINE(prompt);
    LINE("fi");
    /*LINE("if [ \"$UID\" = 0 ] && [ \"$color_prompt\" = yes ]; then");
    prompt = "";
    prompt.append("    PS1=\"$nocolor$PURPLE\\u$nocolor@$CYAN\\h$nocolor: $GREEN\\w$nocolor ");
    if(ui->checkBox_Newline->isChecked())
        prompt.append("\\n");
    prompt.append("\\$ \"");
    LINE(prompt);
    LINE("fi");*/
    if(ui->checkBox_DebianChroot->isChecked())
    {
        LINE("if [ -z debian_chroot ] && [ -r /etc/debian_chroot ]; then");
        LINE("  debian_chroot=$(cat /etc/debian_chroot)");
        LINE("fi");
        LINE("PS1=\"$debian_chroot$PS1\"");
    }
    LINE("#MXBASHRC_END");
#undef LINE

    //Apply changes to file
    QString newCodeCompat;
    foreach (QString i, newCode)
    {
        newCodeCompat += i + "\n";
    }
    content.insert(begin, newCodeCompat);

    bashrc.resize(0);

    stream << content;
    DEBUG_MSG << "--- Window::apply ---";
}

void Window::onAliasButtonAdd()
{
    DEBUG_MSG << "+++ Window::onAliasButtonAdd +++";
    ui->tableWidget->setRowCount(ui->tableWidget->rowCount()+1);
    DEBUG_MSG << "--- Window::onAliasButtonAdd ---";
}

void Window::onAliasButtonRemove()
{
    DEBUG_MSG << "+++ Window::onAliasButtonRemove +++";
    for(int i = ui->tableWidget->rowCount()-1; i > -1; i--)
    {
        if(ui->tableWidget->item(i,0) != nullptr &&  ui->tableWidget->item(i,0)->isSelected())
        {
            ui->tableWidget->removeRow(i);
        }
    }
    DEBUG_MSG << "--- Window::onAliasButtonRemove ---";
}

void Window::onPromptButtonAdd()
{

}

void Window::onPromptButtonRemove()
{

}

void Window::restore()
{
    DEBUG_MSG << "+++ Window::restore +++";
    //Get the default MX Bashrc as your own, in case the program messes  it up
    QFile file(":/mx-linux/mx-linux-bashrc");
    if(!file.open(QFile::ReadOnly | QFile::Text))
        return;
    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();
    QFile bashrc(BASHRC);
    if(!bashrc.open(QFile::WriteOnly | QFile::Text | QFile::Truncate))
        return;
    QTextStream bashrcStream(&bashrc);
    bashrcStream << content;
    bashrc.close();
    DEBUG_MSG << "--- Window::restore ---";
}

QList<PromptToken> PromptParser::parse()
{
    QList<PromptToken> rtn;
    QStringList parts = content.split("$MXBASHRC_PROMPT_PART");
    foreach (QString str, parts) {


    }
}
