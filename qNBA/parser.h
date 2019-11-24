#pragma once


#include <QDate>

extern "C"
{
#include "third_party/tidy-5.6.0-vc14-32b/include/tidy.h"
#include "third_party/tidy-5.6.0-vc14-32b/include/tidybuffio.h"
}

#include <list>
#include <map>

#include "match_info.h"

class Parser
{
public:
    virtual ~Parser() {}

    virtual std::map<QDate, std::list<MatchInfo>> Parse(QByteArray content) = 0;

protected:
    QString GetTidyNodeName(TidyNode node);
    QString GetTidyNodeAttrValue(TidyNode node, QString attrName);
    TidyNode GetFirstChildNode(TidyNode node, QString nodeName,
                               QString attrName = QString(), QString attrValue = QString());
    QString GetTextInNode(TidyDoc tdoc, TidyNode node);
};