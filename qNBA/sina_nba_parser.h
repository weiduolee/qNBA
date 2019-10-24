#pragma once

#include <list>
#include <map>

#include <QDate>

extern "C"
{
#include "third_party/tidy-5.6.0-vc14-32b/include/tidy.h"
#include "third_party/tidy-5.6.0-vc14-32b/include/tidybuffio.h"
}

#include "match_info.h"

class SinaNbaParser
{
public:

    std::map<QDate, std::list<MatchInfo>> Parse(QByteArray content);

private:
    QString GetTidyNodeName(TidyNode node);
    QString GetTidyNodeAttrValue(TidyNode node, QString attrName);
    TidyNode GetFirstChildNode(TidyNode node, QString nodeName,
                               QString attrName = QString(), QString attrValue = QString());
    QString GetTextInNode(TidyDoc tdoc, TidyNode node);

    bool ParseMatchInfo(TidyDoc tdoc, QDate date, TidyNode liNode, MatchInfo* match);
};