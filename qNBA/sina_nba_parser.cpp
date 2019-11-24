#include "sina_nba_parser.h"

std::map<QDate, std::list<MatchInfo>> SinaNbaParser::Parse(QByteArray content)
{
    std::map<QDate, std::list<MatchInfo>> matchList;

    TidyDoc htmlDoc = tidyCreate();

    int ret = tidyParseString(htmlDoc, content);
    if (ret == 0 || ret == 1)
    {
        TidyNode bodyNode = tidyGetBody(htmlDoc);
        TidyNode matchDiv = GetFirstChildNode(bodyNode, "div", "id", "js-match");
        if (matchDiv)
        {
            TidyNode matchConDiv = GetFirstChildNode(matchDiv, "div", "class", "match-con");
            if (matchConDiv)
            {
                TidyNode matchConListDiv = GetFirstChildNode(matchConDiv, "div", "class", "match-con-list");
                if (matchConListDiv)
                {
                    QDate date;
                    for (TidyNode child = tidyGetChild(matchConListDiv); child; child = tidyGetNext(child))
                    {
                        QString value = GetTidyNodeAttrValue(child, "class");
                        if (value == "match-con-date")
                        {
                            QString dateStr = GetTextInNode(htmlDoc, child);
                            dateStr.replace(QString::fromLocal8Bit("Äê"), "-");
                            dateStr.replace(QString::fromLocal8Bit("ÔÂ"), "-");
                            dateStr.replace(QString::fromLocal8Bit("ÈÕ"), "");
                            date = QDate::fromString(dateStr, Qt::ISODate);

                        }
                        else if (date.isValid() && value == "match-con-ul")
                        {
                            TidyNode ulNode = child;
                            for (TidyNode liNode = tidyGetChild(ulNode); liNode; liNode = tidyGetNext(liNode))
                            {
                                if (GetTidyNodeName(liNode) == "li")
                                {
                                    MatchInfo match;
                                    if (ParseMatchInfo(htmlDoc, date, liNode, &match))
                                    {
                                        matchList[date].push_back(match);
                                    }
                                }
                            }
                        }

                    }
                }
            }
        }
    }

    tidyRelease(htmlDoc);

    return matchList;
}

bool SinaNbaParser::ParseMatchInfo(TidyDoc tdoc, QDate date, TidyNode liNode, MatchInfo* match)
{
    if (!match)
        return false;

    TidyNode timeNode = GetFirstChildNode(liNode, "span", "class", "match-con-time living-time2");
    if (!timeNode)
        timeNode = GetFirstChildNode(liNode, "span", "class", "match-con-time");

    if (timeNode)
    {
        QString timeStr = GetTextInNode(tdoc, timeNode);
        QTime time(timeStr.left(2).toInt(), timeStr.mid(3, 2).toInt());
        if (time.isValid())
        {
            match->m_date_time.setDate(date);
            match->m_date_time.setTime(time);
        }
    }

    if (match->m_date_time.isValid())
    {
        TidyNode infoNode = GetFirstChildNode(liNode, "p", "class", "match-con-info");
        if (infoNode)
        {
            TidyNode vsNode = GetFirstChildNode(infoNode, "span", "class", "match-con-vs");
            TidyNode guestTeamNode = GetFirstChildNode(vsNode, "span", "class", "match-con-text_left");
            TidyNode guestLinkNode = tidyGetChild(guestTeamNode);
            QString href = GetTidyNodeAttrValue(guestLinkNode, "href");

            int pos = href.lastIndexOf("?tid=");
            if (pos >= 0 && pos + 5 < href.length())
            {
                match->m_guest_team_sina_id = href.mid(pos + 5);
            }

            match->m_guest_team = GetTextInNode(tdoc, guestLinkNode);

            TidyNode scoreNode = GetFirstChildNode(vsNode, "span", "class", "match-con-num");
            TidyNode scoreLinkNode = tidyGetChild(scoreNode);
            if (scoreLinkNode)
            {
                for (TidyNode child = tidyGetChild(scoreLinkNode); child; child = tidyGetNext(child))
                {
                    if (GetTidyNodeName(child) == "span")
                    {
                        QString scoreStr = GetTextInNode(tdoc, child);
                        bool ok = false;
                        scoreStr.toInt(&ok);
                        if (ok)
                        {
                            match->m_guest_team_score = scoreStr.toInt();
                        }
                        else
                        {
                            match->m_host_team_score = scoreStr.mid(2).toInt();
                        }
                    }
                }
            }

            if (match->m_host_team_score > 0 || match->m_guest_team_score > 0)
                match->m_status = FullTime;

            TidyNode homeLinkNode = GetFirstChildNode(vsNode, "a");
            href = GetTidyNodeAttrValue(homeLinkNode, "href");

            pos = href.lastIndexOf("?tid=");
            if (pos >= 0 && pos + 5 < href.length())
            {
                match->m_host_team_sina_id = href.mid(pos + 5);
            }

            match->m_host_team = GetTextInNode(tdoc, homeLinkNode);
            return true;
        }
        else
        {
            infoNode = GetFirstChildNode(liNode, "p", "class", "match-con-info match-con-nobegin");
            for (TidyNode child = tidyGetChild(infoNode); child; child = tidyGetNext(child))
            {
                if (GetTidyNodeName(child) == "span")
                {
                    if (GetTidyNodeAttrValue(child, "class") == "match-con-text_left")
                    {
                        TidyNode guestTeamNode = child;
                        TidyNode guestLinkNode = tidyGetChild(guestTeamNode);
                        QString href = GetTidyNodeAttrValue(guestLinkNode, "href");

                        int pos = href.lastIndexOf("?tid=");
                        if (pos >= 0 && pos + 5 < href.length())
                        {
                            match->m_guest_team_sina_id = href.mid(pos + 5);
                        }

                        match->m_guest_team = GetTextInNode(tdoc, guestLinkNode);
                    }
                    else if ((GetTidyNodeAttrValue(child, "class") == "match-con-line2"))
                    {

                    }
                    else
                    {
                        TidyNode homeLinkNode = GetFirstChildNode(child, "a");
                        QString href = GetTidyNodeAttrValue(homeLinkNode, "href");

                        int pos = href.lastIndexOf("?tid=");
                        if (pos >= 0 && pos + 5 < href.length())
                        {
                            match->m_host_team_sina_id = href.mid(pos + 5);
                        }

                        match->m_host_team = GetTextInNode(tdoc, homeLinkNode);
                    }
                }
            }
            return true;
        }
    }

    return false;
}