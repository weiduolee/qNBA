#include "parser.h"


QString Parser::GetTidyNodeName(TidyNode node)
{
    QString name;
    switch (tidyNodeGetType(node))
    {
    case TidyNode_Root:       name = "Root";                    break;
    case TidyNode_DocType:    name = "DOCTYPE";                 break;
    case TidyNode_Comment:    name = "Comment";                 break;
    case TidyNode_ProcIns:    name = "Processing Instruction";  break;
    case TidyNode_Text:       name = "Text";                    break;
    case TidyNode_CDATA:      name = "CDATA";                   break;
    case TidyNode_Section:    name = "XML Section";             break;
    case TidyNode_Asp:        name = "ASP";                     break;
    case TidyNode_Jste:       name = "JSTE";                    break;
    case TidyNode_Php:        name = "PHP";                     break;
    case TidyNode_XmlDecl:    name = "XML Declaration";         break;

    case TidyNode_Start:
    case TidyNode_End:
    case TidyNode_StartEnd:
    default:
        name = tidyNodeGetName(node);
        break;
    }

    return name;
}

QString Parser::GetTidyNodeAttrValue(TidyNode node, QString attrName)
{
    if (!node || attrName.isEmpty())
        return QString();

    for (TidyAttr attr = tidyAttrFirst(node); attr; attr = tidyAttrNext(attr))
    {
        QString name = tidyAttrName(attr);
        if (name == attrName)
        {
            return tidyAttrValue(attr);
        }
    }

    return QString();
}

TidyNode Parser::GetFirstChildNode(TidyNode node, QString nodeName,
                                          QString attrName, QString attrValue)
{
    if (!node || nodeName.isEmpty())
        return nullptr;

    TidyNode matchNode = nullptr;
    for (TidyNode child = tidyGetChild(node); child; child = tidyGetNext(child))
    {
        QString name = GetTidyNodeName(child);
        if (name == nodeName)
        {
            if (!attrName.isEmpty() && !attrValue.isEmpty())
            {
                QString value = GetTidyNodeAttrValue(child, attrName);
                if (value == attrValue)
                {
                    matchNode = child;
                    break;
                }
            }
            else
            {
                matchNode = child;
                break;
            }
        }
    }

    return matchNode;
}

QString Parser::GetTextInNode(TidyDoc tdoc, TidyNode node)
{
    TidyNode textNode = tidyGetChild(node);
    if (tidyNodeGetType(textNode) == TidyNode_Text && tidyNodeHasText(tdoc, textNode))
    {
        TidyBuffer text;
        tidyBufInit(&text);
        if (tidyNodeGetText(tdoc, textNode, &text))
        {
            return QString((char*)text.bp).trimmed();
        }
    }

    return QString();
}
