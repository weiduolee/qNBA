#pragma once

#include <QDateTime>
#include <QString>

enum MatchStatus
{
    NotStart = 0,
    Playing,
    FullTime,
};

struct MatchInfo
{
    MatchStatus m_status = NotStart;
    QDateTime m_date_time;
    QString m_host_team;
    QString m_host_team_sina_id;
    QString m_guest_team;
    QString m_guest_team_sina_id;
    int m_host_team_score = 0;
    int m_guest_team_score = 0;
    int m_ot = 0;
};