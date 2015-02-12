#include "queries.hpp"

QList<Query*> Queries::sortedQueries(int(*sort_callback)(Query*, Query*)){
    QList<Query*> queries = values();
    qSort(queries.begin(), queries.end(), sort_callback);
    return queries;
}
