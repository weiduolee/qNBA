#include "qNBA.h"

#include <windows.h>

#include <QDate>
#include <QFile>
#include <QMessageBox>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>

#include "sina_nba_parser.h"
#include "sina_nba_team_id.h"

qNBA::qNBA(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    InitNbaLogo();
    InitMatchListUI();

    connect(ui.toolButtonImport, SIGNAL(clicked()), this, SLOT(onToolButtonImportClicked()));
    connect(ui.toolButtonDownload, SIGNAL(clicked()), this, SLOT(onToolButtonDowloadClicked()));
    connect(ui.tableWidgetMatchList, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(onTableWidgetMatchListCellDoubleClicked(int, int)));
}

void qNBA::onToolButtonDowloadClicked()
{
    QDate today = QDate::currentDate();

    QString program = R"(C:\Projects\cef\cef_binary_77.1.14\build\tests\cefsimple\Debug\cefsimple.exe)";
    QString url = R"(--url="https://slamdunk.sports.sina.com.cn/match#status=1&date=)";
    url += today.toString(Qt::ISODate);
    url += "\"";

    QString output_file = R"( --output_html=")";
    output_file += QCoreApplication::applicationDirPath();
    output_file += "/output_html/";
    output_file += today.toString(Qt::ISODate);
    output_file += ".txt\"";

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::wstring cmd = program.toStdWString();
    cmd += L" ";
    cmd += url.toStdWString();
    cmd += L" ";
    cmd += output_file.toStdWString();
    BOOL ret = CreateProcess(NULL, (LPWSTR)cmd.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
}

void qNBA::onToolButtonImportClicked()
{
    QString output_file = QCoreApplication::applicationDirPath();
    output_file += "/output_html/";
    output_file += QDate::currentDate().toString(Qt::ISODate);
    output_file += ".txt";

    // Load xml file as raw data
    QFile f(output_file);
    if (!f.open(QIODevice::ReadOnly))
    {
        // Error while loading file
        int ret = QMessageBox::warning(this, "", "Failed to load html file.");
        return;
    }

    QByteArray content = f.readAll();

    SinaNbaParser sina;
    std::map<QDate, std::list<MatchInfo>> matchList = sina.Parse(content);
    
    f.close();

    UpdateDB(matchList);

    UpdateUI(matchList);
}

void qNBA::onTableWidgetMatchListCellDoubleClicked(int row, int column)
{
    if (currentRow != row)
    {
        currentRow = row;

        QTableWidgetItem* item1 = ui.tableWidgetMatchList->item(row, 0);
        QTableWidgetItem* item2 = ui.tableWidgetMatchList->item(row, 1);
        QTableWidgetItem* item3 = ui.tableWidgetMatchList->item(row, 2);
        QTableWidgetItem* item4 = ui.tableWidgetMatchList->item(row, 3);
        if (item1 && !item1->text().isEmpty()
            && item2 && !item2->text().isEmpty()
            && item3 && !item3->text().isEmpty()
            && item4 && !item4->text().isEmpty())
        {
            QDateTime date_time = item1->data(Qt::UserRole).toDateTime();
            QString guest_team_sina_id = item2->data(Qt::UserRole).toString();
            QString host_team_sina_id = item4->data(Qt::UserRole).toString();

            ResetMatchListUI();

            ui.labelGuestLogo->setPixmap(m_logo_map[guest_team_sina_id]);
            ui.labelHostLogo->setPixmap(m_logo_map[host_team_sina_id]);

            UpdateUIFromDBData(date_time, guest_team_sina_id, host_team_sina_id);
        }
    }
}

bool qNBA::UpdateDB(std::map<QDate, std::list<MatchInfo>>& matchList)
{
    if (OpenDB())
    {
        for (auto match : matchList)
        {
            QSqlQuery query;
            QString statement = "select * from match where date = '" + match.first.toString(Qt::ISODate) + "'";
            bool res = query.exec(statement);
            if (!res)
            {
                QSqlError error = m_nba_db.lastError();
                QString text = error.databaseText() + ", " + error.driverText() + ", " +
                    error.nativeErrorCode() + ", " + error.text();
                QMessageBox::warning(this, "Debug", text);
                return false;
            }

            if (query.next())
                continue;

            for (MatchInfo& info : match.second)
            {
                if (info.m_status != FullTime)
                    continue;

                query.prepare("INSERT INTO match (status, date, time, host_team, host_team_sina_id, host_team_score, " \
                                                  "guest_team, guest_team_sina_id, guest_team_score) "
                              "VALUES (:status, :date, :time, :host_team, :host_team_sina_id, :host_team_score, :guest_team, :guest_team_sina_id, :guest_team_score)");
                query.bindValue(":status", info.m_status);
                query.bindValue(":date", info.m_date_time.date().toString(Qt::ISODate));
                query.bindValue(":time", info.m_date_time.time().toString("hh:mm"));
                query.bindValue(":host_team", info.m_host_team);
                query.bindValue(":host_team_sina_id", info.m_host_team_sina_id);
                query.bindValue(":host_team_score", info.m_host_team_score);
                query.bindValue(":guest_team", info.m_guest_team);
                query.bindValue(":guest_team_sina_id", info.m_guest_team_sina_id);
                query.bindValue(":guest_team_score", info.m_guest_team_score);

                res = query.exec();
                if (!res)
                {
                    QSqlError error = m_nba_db.lastError();
                    QString text = error.databaseText() + ", " + error.driverText() + ", " +
                        error.nativeErrorCode() + ", " + error.text();
                    QMessageBox::warning(this, "Debug", text);
                    return false;
                }
            }

        }

        return true;
    }

    return false;
}

void qNBA::UpdateUI(std::map<QDate, std::list<MatchInfo>>& matchList)
{
    for (std::pair<QDate, std::list<MatchInfo>> item : matchList)
    {
        ui.tableWidgetMatchList->insertRow(ui.tableWidgetMatchList->rowCount());

        int row = ui.tableWidgetMatchList->rowCount() - 1;

        QTableWidgetItem* dateItem1 = new QTableWidgetItem(item.first.toString(Qt::ISODate));
        dateItem1->setBackgroundColor(QColor::fromRgb(150, 150, 150));
        ui.tableWidgetMatchList->setItem(row, 0, dateItem1);

        QTableWidgetItem* dateItem2 = new QTableWidgetItem();
        dateItem2->setBackgroundColor(QColor::fromRgb(150, 150, 150));
        ui.tableWidgetMatchList->setItem(row, 1, dateItem2);

        QTableWidgetItem* dateItem3 = new QTableWidgetItem();
        dateItem3->setBackgroundColor(QColor::fromRgb(150, 150, 150));
        ui.tableWidgetMatchList->setItem(row, 2, dateItem3);

        QTableWidgetItem* dateItem4 = new QTableWidgetItem();
        dateItem4->setBackgroundColor(QColor::fromRgb(150, 150, 150));
        ui.tableWidgetMatchList->setItem(row, 3, dateItem4);

        for (MatchInfo& info : item.second)
        {
            ui.tableWidgetMatchList->insertRow(ui.tableWidgetMatchList->rowCount());
            row = ui.tableWidgetMatchList->rowCount() - 1;

            QString time = info.m_date_time.toString("hh:mm");
            QTableWidgetItem* timeItem = new QTableWidgetItem(time);
            timeItem->setData(Qt::UserRole, info.m_date_time);
            ui.tableWidgetMatchList->setItem(row, 0, timeItem);

            QTableWidgetItem* guestItem = new QTableWidgetItem(info.m_guest_team);
            guestItem->setData(Qt::UserRole, info.m_guest_team_sina_id);
            ui.tableWidgetMatchList->setItem(row, 1, guestItem);

            QString score = "-";
            if (info.m_status == FullTime)
                score = QString::number(info.m_guest_team_score) + " - " + QString::number(info.m_host_team_score);

            QTableWidgetItem* scoreItem = new QTableWidgetItem(score);
            scoreItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            ui.tableWidgetMatchList->setItem(row, 2, scoreItem);

            QTableWidgetItem* hostItem = new QTableWidgetItem(info.m_host_team);
            hostItem->setData(Qt::UserRole, info.m_host_team_sina_id);
            ui.tableWidgetMatchList->setItem(row, 3, hostItem);
        }

        ui.tableWidgetMatchList->insertRow(ui.tableWidgetMatchList->rowCount());
    }

    QDate date = QDate::currentDate();
    ui.labelDay7->setText(date.toString("MM-dd"));
    date = date.addDays(-1);
    ui.labelDay6->setText(date.toString("MM-dd"));
    date = date.addDays(-1);
    ui.labelDay5->setText(date.toString("MM-dd"));
    date = date.addDays(-1);
    ui.labelDay4->setText(date.toString("MM-dd"));
    date = date.addDays(-1);
    ui.labelDay3->setText(date.toString("MM-dd"));
    date = date.addDays(-1);
    ui.labelDay2->setText(date.toString("MM-dd"));
    date = date.addDays(-1);
    ui.labelDay1->setText(date.toString("MM-dd"));
}

bool qNBA::OpenDB()
{
    if (m_nba_db.isOpen())
        return true;

    QString nba_db_path = QCoreApplication::applicationDirPath();
    nba_db_path.append("/nba.db");

    m_nba_db = QSqlDatabase::addDatabase("QSQLITE");
    m_nba_db.setDatabaseName(nba_db_path);
    bool res = m_nba_db.open();
    if (!res)
    {
        QSqlError error = m_nba_db.lastError();
        QString text = error.databaseText() + ", " + error.driverText() + ", " +
            error.nativeErrorCode() + ", " + error.text();
        QMessageBox::warning(this, "Debug", text);
        return false;
    }

    return true;
}

void qNBA::UpdateUIFromDBData(QDateTime date_time, QString guest_sina_id, QString host_sina_id)
{
    QDate today = QDate::currentDate();
    QString win_style = "background-color: rgb(0, 255, 0);";
    QString lose_style = "background-color: rgb(255, 0, 0);";

    // guest team
    QSqlQuery query_guest;
    QString statement_guest = "SELECT * FROM match WHERE guest_team_sina_id = '" + guest_sina_id
        + "' OR host_team_sina_id = '" + guest_sina_id + "' ORDER BY id DESC ";
    bool res = query_guest.exec(statement_guest);
    if (!res)
    {
        QSqlError error = m_nba_db.lastError();
        QString text = error.databaseText() + ", " + error.driverText() + ", " +
            error.nativeErrorCode() + ", " + error.text();
        QMessageBox::warning(this, "Debug", text);
        return;
    }
    
    while (query_guest.next())
    {
        QSqlRecord record = query_guest.record();

        QDate date = record.value("date").toDate();

        QString  guest_team = record.value("guest_team").toString();
        QString  host_team = record.value("host_team").toString();

        QString  guest_team_sina_id = record.value("guest_team_sina_id").toString();
        QString  host_team_sina_id = record.value("host_team_sina_id").toString();

        int  guest_team_score = record.value("guest_team_score").toInt();
        int  host_team_score = record.value("host_team_score").toInt();

        int ot = record.value("ot").toInt();

        QString label_text = guest_team;
        label_text += " ";
        label_text += QString::number(guest_team_score);
        label_text += "\n";
        label_text += host_team;
        label_text += " ";
        label_text += QString::number(host_team_score);
        label_text += "\n\nOT ";
        label_text += QString::number(ot);;

        QDate day2000(2000, 1, 1);
        int diff = date.daysTo(day2000) - today.daysTo(day2000);
        if (guest_sina_id == guest_team_sina_id)
        {
            bool win = guest_team_score > host_team_score;
            if (diff >= 0 && diff < m_label_guest_1.size())
            {
                QLabel* label = m_label_guest_1[diff];
                label->setText(label_text);
                label->setStyleSheet(win ? win_style : lose_style);
            }
            else
            {
                break;
            }
        }
        else
        {
            bool win = guest_team_score < host_team_score;
            if (diff >= 0 && diff < m_label_guest_2.size())
            {
                QLabel* label = m_label_guest_2[diff];
                label->setText(label_text);
                label->setStyleSheet(win ? win_style : lose_style);
            }
            else
            {
                break;
            }
        }
    }

    // host team
    QSqlQuery query_host;
    QString statement_host = "SELECT * FROM match WHERE guest_team_sina_id = '" + host_sina_id
        + "' OR host_team_sina_id = '" + host_sina_id + "' ORDER BY id DESC ";
    res = query_host.exec(statement_host);
    if (!res)
    {
        QSqlError error = m_nba_db.lastError();
        QString text = error.databaseText() + ", " + error.driverText() + ", " +
            error.nativeErrorCode() + ", " + error.text();
        QMessageBox::warning(this, "Debug", text);
        return;
    }

    while (query_host.next())
    {
        QSqlRecord record = query_host.record();

        QDate date = record.value("date").toDate();

        QString  guest_team = record.value("guest_team").toString();
        QString  host_team = record.value("host_team").toString();

        QString  guest_team_sina_id = record.value("guest_team_sina_id").toString();
        QString  host_team_sina_id = record.value("host_team_sina_id").toString();

        int  guest_team_score = record.value("guest_team_score").toInt();
        int  host_team_score = record.value("host_team_score").toInt();

        int ot = record.value("ot").toInt();

        QString label_text = guest_team;
        label_text += " ";
        label_text += QString::number(guest_team_score);
        label_text += "\n";
        label_text += host_team;
        label_text += " ";
        label_text += QString::number(host_team_score);
        label_text += "\n\nOT ";
        label_text += QString::number(ot);;

        QDate day2000(2000, 1, 1);
        int diff = date.daysTo(day2000) - today.daysTo(day2000);
        if (host_sina_id == guest_team_sina_id)
        {
            bool win = guest_team_score > host_team_score;
            if (diff >= 0 && diff < m_label_host_1.size())
            {
                QLabel* label = m_label_host_1[diff];
                label->setText(label_text);
                label->setStyleSheet(win ? win_style : lose_style);
            }
            else
            {
                break;
            }
        }
        else
        {
            bool win = guest_team_score < host_team_score;
            if (diff >= 0 && diff < m_label_host_2.size())
            {
                QLabel* label = m_label_host_2[diff];
                label->setText(label_text);
                label->setStyleSheet(win ? win_style : lose_style);
            }
            else
            {
                break;
            }
        }
    }
}

void qNBA::InitNbaLogo()
{
    m_logo_map[atl] = QPixmap(":/qNBA/res/atl.png");
    m_logo_map[bkn] = QPixmap(":/qNBA/res/bkn.png");
    m_logo_map[bos] = QPixmap(":/qNBA/res/bos.png");
    m_logo_map[cha] = QPixmap(":/qNBA/res/cha.png");
    m_logo_map[chi] = QPixmap(":/qNBA/res/chi.png");
    m_logo_map[cle] = QPixmap(":/qNBA/res/cle.png");
    m_logo_map[dal] = QPixmap(":/qNBA/res/dal.png");
    m_logo_map[den] = QPixmap(":/qNBA/res/den.png");
    m_logo_map[det] = QPixmap(":/qNBA/res/det.png");
    m_logo_map[gs] = QPixmap(":/qNBA/res/gs.png");
    m_logo_map[hou] = QPixmap(":/qNBA/res/hou.png");
    m_logo_map[ind] = QPixmap(":/qNBA/res/ind.png");
    m_logo_map[lac] = QPixmap(":/qNBA/res/lac.png");
    m_logo_map[lal] = QPixmap(":/qNBA/res/lal.png");
    m_logo_map[mem] = QPixmap(":/qNBA/res/mem.png");
    m_logo_map[mia] = QPixmap(":/qNBA/res/mia.png");
    m_logo_map[mil] = QPixmap(":/qNBA/res/mil.png");
    m_logo_map[min] = QPixmap(":/qNBA/res/min.png");
    m_logo_map[nor] = QPixmap(":/qNBA/res/nor.png");
    m_logo_map[ny] = QPixmap(":/qNBA/res/ny.png");
    m_logo_map[okc] = QPixmap(":/qNBA/res/okc.png");
    m_logo_map[orl] = QPixmap(":/qNBA/res/orl.png");
    m_logo_map[phi] = QPixmap(":/qNBA/res/phi.png");
    m_logo_map[pho] = QPixmap(":/qNBA/res/pho.png");
    m_logo_map[por] = QPixmap(":/qNBA/res/por.png");
    m_logo_map[sa] = QPixmap(":/qNBA/res/sa.png");
    m_logo_map[sac] = QPixmap(":/qNBA/res/sac.png");
    m_logo_map[tor] = QPixmap(":/qNBA/res/tor.png");
    m_logo_map[utah] = QPixmap(":/qNBA/res/utah.png");
    m_logo_map[was] = QPixmap(":/qNBA/res/was.png");
}

void qNBA::InitMatchListUI()
{
    m_label_guest_1.push_back(ui.labelGuest7);
    m_label_guest_1.push_back(ui.labelGuest6);
    m_label_guest_1.push_back(ui.labelGuest5);
    m_label_guest_1.push_back(ui.labelGuest4);
    m_label_guest_1.push_back(ui.labelGuest3);
    m_label_guest_1.push_back(ui.labelGuest2);
    m_label_guest_1.push_back(ui.labelGuest1);

    m_label_guest_2.push_back(ui.labelGuest7_2);
    m_label_guest_2.push_back(ui.labelGuest6_2);
    m_label_guest_2.push_back(ui.labelGuest5_2);
    m_label_guest_2.push_back(ui.labelGuest4_2);
    m_label_guest_2.push_back(ui.labelGuest3_2);
    m_label_guest_2.push_back(ui.labelGuest2_2);
    m_label_guest_2.push_back(ui.labelGuest1_2);

    m_label_host_1.push_back(ui.labelHost7);
    m_label_host_1.push_back(ui.labelHost6);
    m_label_host_1.push_back(ui.labelHost5);
    m_label_host_1.push_back(ui.labelHost4);
    m_label_host_1.push_back(ui.labelHost3);
    m_label_host_1.push_back(ui.labelHost2);
    m_label_host_1.push_back(ui.labelHost1);

    m_label_host_2.push_back(ui.labelHost7_2);
    m_label_host_2.push_back(ui.labelHost6_2);
    m_label_host_2.push_back(ui.labelHost5_2);
    m_label_host_2.push_back(ui.labelHost4_2);
    m_label_host_2.push_back(ui.labelHost3_2);
    m_label_host_2.push_back(ui.labelHost2_2);
    m_label_host_2.push_back(ui.labelHost1_2);

    ResetMatchListUI();
}

void qNBA::ResetMatchListUI()
{
    ui.labelGuestLogo->setPixmap(QPixmap());
    ui.labelHostLogo->setPixmap(QPixmap());

    for (QLabel* label : m_label_guest_1)
    {
        label->setText("");
        label->setStyleSheet("");
    }

    for (QLabel* label : m_label_guest_2)
    {
        label->setText("");
        label->setStyleSheet("");
    }

    for (QLabel* label : m_label_host_1)
    {
        label->setText("");
        label->setStyleSheet("");
    }

    for (QLabel* label : m_label_host_2)
    {
        label->setText("");
        label->setStyleSheet("");
    }
}