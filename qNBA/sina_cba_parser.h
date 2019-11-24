#pragma once


#include "parser.h"

class SinaCbaParser : public Parser
{
public:
    std::map<QDate, std::list<MatchInfo>> Parse(QByteArray content) override;

private:
    bool ParseMatchInfo(TidyDoc tdoc, QDate date, TidyNode liNode, MatchInfo* match);
};