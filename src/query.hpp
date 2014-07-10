#ifndef QUERY_H
#define QUERY_H

#include <QObject>
#include <QHash>
#include <QRegExp>
#include <QStringList>

class Query : public QObject{
public:
    Query(QString normalized_statement, QString statement, QString user, QString database, uint duration);
    void setStatement(QString statement);
    void setUser(QString user);
    void setDatabase(QString database);
    void addStatement(uint duration, QString statement);
    QString getStatement(){return statement;}
    QString getUser(){return user;}
    QString getDatabase(){return database;}
    QList<uint> getDurations(){return durations;}
    QStringList getExamples(){return examples;}
    uint getTotalDuration(){return totalDuration;}
    uint getAverageDuration();
    uint getExecutions(){return executions;}
    static QString normalize(QString statement);
    static QString format(QString statement);

private:
    QString statement;
    QString user;
    QString database;
    QList<uint> durations;
    QStringList examples;
    uint executions;
    uint totalDuration;
    void init();
};

#endif // QUERY_H
