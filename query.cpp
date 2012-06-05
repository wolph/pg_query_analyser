#include "query.h"

Query::Query(QString normalized_statement, QString statement, QString user, QString database, uint duration)
{
    this->executions = 0;
    this->totalDuration = 0;
    this->setStatement(normalized_statement);
    this->setUser(user);
    this->setDatabase(database);
    this->addStatement(duration, statement);
}

void Query::setStatement(QString statement){
    this->statement = statement;
}

void Query::setUser(QString user){
    this->user = user;
}

void Query::setDatabase(QString database){
    this->database = database;
}

void Query::addStatement(uint duration, QString statement){
    totalDuration += duration;
    executions++;
    durations.append(duration);
    if(examples.size() < 25)
        examples.append(statement);
}

uint Query::getAverageDuration(){
    return this->totalDuration / this->executions;
}

// QRegExp filter_digits("([^_a-zA-Z0-9])\\d+");
pcrecpp::RE filter_digits("([^_a-zA-Z0-9])\\d+");
pcrecpp::RE filter_limit("LIMIT \\d+");
pcrecpp::RE filter_operator("\\s*(!=|>=|<=|!=|<|>|=)\\s*(\\d+|E?'[^']*')");
pcrecpp::RE filter_in(" (in|IN)\\s+\\([^\\)]+(\\)|$)");
pcrecpp::RE newline_match("\\s+(FROM|WHERE|LIMIT|GROUP BY|ORDER BY|LEFT JOIN|RIGHT JOIN|LEFT OUTER JOIN|RIGHT OUTER JOIN|JOIN)");
pcrecpp::RE tabnewline_match("\\s+(AND|OR)");
pcrecpp::RE comma_match(", ");
pcrecpp::RE whitespace_match("\\s+");

QString Query::normalize(QString statement){
    string stmt = statement.toUtf8().constData();
    normalize(&stmt);
    return QString::fromStdString(stmt);
}

void Query::normalize(string *statement){
    whitespace_match.GlobalReplace(" ", statement);

    filter_in.GlobalReplace(" IN (N)", statement);
    filter_limit.GlobalReplace("LIMIT N", statement);
    filter_operator.GlobalReplace(" \\1 N", statement);
    filter_digits.GlobalReplace("\\1N", statement);
    format(statement);
}

QString Query::format(QString statement){
    string stmt = statement.toUtf8().constData();
    format(&stmt);
    return QString::fromStdString(stmt);
}

void Query::format(string *statement){
    comma_match.GlobalReplace(",\n\t", statement);
    newline_match.GlobalReplace("\n\\1", statement);
    tabnewline_match.GlobalReplace("\n\t\\1", statement);
}

