#include "sina_cba_parser.h"

std::map<QDate, std::list<MatchInfo>> SinaCbaParser::Parse(QByteArray content)
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

bool SinaCbaParser::ParseMatchInfo(TidyDoc tdoc, QDate date, TidyNode liNode, MatchInfo* match)
{
    if (!match)
        return false;

    return true;
}