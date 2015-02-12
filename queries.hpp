#ifndef QUERIES_H
#define QUERIES_H

#include <QHash>
#include <QList>
#include <query.hpp>

class Queries : public QHash<uint, Query*>
{
public:
    QList<Query*> sortedQueries(int(*sort_callback)(Query*, Query*));
    static int mostExecutions(Query *a, Query *b){return a->getExecutions() > b->getExecutions();}
    static int mostTotalDuration(Query *a, Query *b){return a->getTotalDuration() > b->getTotalDuration();}
    static int mostAverageDuration(Query *a, Query *b){return a->getAverageDuration() > b->getAverageDuration();}
};

#endif // QUERIES_H
