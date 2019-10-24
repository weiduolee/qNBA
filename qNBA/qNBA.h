#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_qNBA.h"

#include <vector>

#include <QSqlDatabase>

#include "match_info.h"

class qNBA : public QMainWindow
{
    Q_OBJECT

public:
    qNBA(QWidget *parent = Q_NULLPTR);

private slots:
    void onToolButtonDowloadClicked();
    void onToolButtonImportClicked();
    void onTableWidgetMatchListCellDoubleClicked(int row, int column);

private:
    bool OpenDB();
    bool UpdateDB(std::map<QDate, std::list<MatchInfo>>& matchList);
    void UpdateUI(std::map<QDate, std::list<MatchInfo>>& matchList);

    void UpdateUIFromDBData(QDateTime date_time, QString guest_sina_id, QString host_sina_id);

    void InitNbaLogo();
    void InitMatchListUI();
    void ResetMatchListUI();

    Ui::qNBAClass ui;

    std::vector<QLabel*> m_label_guest_1;
    std::vector<QLabel*> m_label_guest_2;

    std::vector<QLabel*> m_label_host_1;
    std::vector<QLabel*> m_label_host_2;

    std::map<QString, QPixmap> m_logo_map;

    int currentRow = 0;

    QSqlDatabase m_nba_db;
};
