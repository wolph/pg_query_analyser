#include "main.h"

int min(int a, int b){
    return a < b ? a : b;
}

int example = 0;

QString escape(QString arg){
    arg = arg.replace(">", "&gt;");
    arg = arg.replace(">", "&lt;");
    return arg;
}

QString print_queries(QList<Query*> queries, int top){
    QString output_string;
    QTextStream output(&output_string, QIODevice::WriteOnly);
    output <<"<table class=\"queryList\">"
        "<tr>"
          "<th>Total duration</th>"
          "<th>Executions</th>"
          "<th>Average duration</th>"
          "<th>User</th>"
          "<th>Database</th>"
          "<th>Query</th>"
        "</tr>";

    for(int i=0; i<min(queries.size(), top); i++){
        Query *q = queries.at(i);
        output << QString("<tr class=\"row%1\">"
          "<td class=\"relevantInformation top center\">%3 ms</td>"
          "<td class=\"top center\">%4</td>"
          "<td class=\"top center\">%5 ms</td>"
          "<td class=\"top center\">%6</td>"
          "<td class=\"top center\">%7</td>"
          "<td><pre onclick=\"highlight(this);\">%8</pre></td></tr>"
          "<tr><td colspan=\"6\">"
          "<input type=\"button\" class=\"examplesButton\" value=\"Show examples\" "
          "onclick=\"javascript:toggleExamples(this, %9);\" />"
          "<div id=\"example_%9\" \"class=\"examples c1\" style=\"display: none;\">") \
            .arg(i%2) \
            .arg(q->getTotalDuration() / 1000) \
            .arg(q->getExecutions()) \
            .arg(q->getAverageDuration() / 1000) \
            .arg(q->getUser()) \
            .arg(q->getDatabase()) \
            .arg(escape(q->getStatement())) \
            .arg(example);

        QStringList examples = q->getExamples();
        QList<uint> durations = q->getDurations();
        for(int j=0; j<examples.count(); j++){
            output << QString("<div class=\"example%1 sql\">%2ms | <pre onclick=\"highlight(this);\">%3</pre></div>") \
                .arg(j % 2) \
                .arg(durations.at(j)) \
                .arg(escape(examples.at(j)));
        }

        output << "</div></td></tr>";
        example++;
    }
    output << "</table>";

    return output_string;
}

int main(int argc, char **argv){
    QTextStream qout(stdout, QIODevice::WriteOnly);
    QTextStream qerr(stderr, QIODevice::WriteOnly);
    QTextStream qin(stdin, QIODevice::ReadOnly);

    Args args;
    args.add(new Arg("v", "verbose", Arg::setTrue, QVariant(false)));
    //args.add(new Arg("i", "input-file", Arg::readableFile, QVariant("/var/log/postgresql.log")));
    args.add(new Arg("i", "input-file", Arg::readableFile,
        QVariant("/var/log/postgresql/postgresql-9.1-main.log")));
    args.add(new Arg("o", "output-file", Arg::writableFile,
        QVariant("report.html")));
    args.add(new Arg("u", "users", Arg::toString, QVariant()));
    args.add(new Arg("d", "databases", Arg::toString, QVariant()));
    args.add(new Arg("top", Arg::toInt, QVariant(20)));
    args.add(new Arg("t", "query-types", Arg::toString,
        QVariant("SELECT,UPDATE,INSERT,DELETE")));

    if(!args.parse(argc, argv)){
        args.help();
        return -1;
    }

    QStringList users = args.getStringList("users");
    QStringList databases = args.getStringList("databases");
    QStringList query_types = args.getStringList("query-types");

    int ret;

    QString line;
    int start_index;
    int stop_index;

    int old_query_id;
    int new_query_id;
    int old_line_id;
    int new_line_id;
    QString database;
    QString user;
    QString statement;
    uint duration;
    QFile input_file;
    ret = args.getFile(&input_file, "input-file", QIODevice::ReadOnly | QIODevice::Text);
    if(ret)return ret;
    QFile output_file;
    ret = args.getFile(&output_file, "output-file", QIODevice::WriteOnly | QIODevice::Text);
    if(ret)return ret;
    QTextStream output(&output_file);

    /* display the top N queries */
    int top = args.getInt("top");

    Queries queries;
    old_query_id = -1;
    old_line_id = -0;
    statement = "";
    duration = 0;

    QTime timer;
    timer.start();

    uint lines = 0;

    if(input_file.atEnd()){
        qerr << "The input file (" << input_file.fileName();
        qerr << ") seems to be empty" << endl;
    }

    while (!input_file.atEnd()) {
        line = input_file.readLine(4096);
        lines++;
        if(lines % 1000 == 0){
            qout << "Read " << lines << " lines." << endl;
            qout.flush();
        }

        if(line[0] == '\t'){
            statement.append(line);
            continue;
        }

        start_index = line.indexOf("[", 15);
        start_index = line.indexOf("[", start_index + 3) + 1;
        stop_index = line.indexOf("-", start_index);

        new_query_id = line.mid(start_index, stop_index - start_index).toInt();

        start_index = stop_index + 1;
        stop_index = line.indexOf("]", start_index);

        new_line_id = line.mid(start_index, stop_index - start_index).toInt();

        if(new_query_id != old_query_id || old_line_id < new_line_id){
            old_query_id = new_query_id;
            QString hashStatement = Query::normalize(statement);
            statement = Query::format(statement);
            if((
                    hashStatement.startsWith("INSERT") ||
                    hashStatement.startsWith("DELETE") ||
                    hashStatement.startsWith("UPDATE") ||
                    hashStatement.startsWith("SELECT")
                )
                    && (!users.length() || users.contains(user))
                    && (!databases.length() || databases.contains(database))){
                uint hash = qHash(hashStatement);
                if(queries.contains(hash)){
                    queries[hash]->addStatement(duration, statement);
                }else{
                    queries.insert(hash, new Query(hashStatement, statement, user, database, duration));
                }
            }

            user = "";
            database = "";
            duration = 0;
            statement = "";

            start_index = line.indexOf("user=", stop_index) + 5;
            stop_index = line.indexOf(",", start_index);
            user = line.mid(start_index, stop_index - start_index);

            start_index = line.indexOf("db=", stop_index) + 3;
            stop_index = line.indexOf(" ", start_index);
            if(start_index == -1 || stop_index == -1)continue;
            database = line.mid(start_index, stop_index - start_index);

            start_index = line.indexOf("duration: ", stop_index) + 10;
            stop_index = line.indexOf(" ", start_index);
            duration = line.mid(start_index, stop_index - start_index).toDouble() * 1000;

            start_index = line.indexOf("statement: ", stop_index) + 11;
            if(start_index < stop_index)continue;
            stop_index = line.length();
            statement = line.mid(start_index, stop_index - start_index);
        }else{
            start_index = line.indexOf("] ", stop_index);
            if(start_index != -1){
                start_index += 2;

                stop_index = line.length() - start_index;
                statement.append(line.mid(start_index, line.length() - start_index));
            }
        }
    }

    QFile header("header.html");
    if (!header.open(QIODevice::ReadOnly | QIODevice::Text)){
        qout << "Unable to open header.html" << endl;
        return -4;
    }else{
        output << header.readAll();
        header.close();
    }
    QList<Query*> queries_sorted;

    output << QString("<div class=\"information\">"
      "<ul>"
        "<li>Generated on %1</li>"
        "<li>Parsed %2 (%3 lines) in %4 ms</li>"
      "</ul>"
    "</div>") \
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")) \
        .arg(args.getString("input_file")) \
        .arg(lines) \
        .arg(timer.elapsed());

    output << "<div class=\"reports\">";

    output << "<h2 id=\"NormalizedQueriesMostTimeReport\">Queries that took up the most time (N) <a href=\"#top\" title=\"Back to top\">^</a></h2>";
    output << print_queries(queries.sortedQueries(Queries::mostTotalDuration), top);

    output << "<h2 id=\"NormalizedQueriesMostFrequentReport\">Most frequent queries (N) <a href=\"#top\" title=\"Back to top\">^</a></h2>";
    output << print_queries(queries.sortedQueries(Queries::mostExecutions), top);

    output << "<h2 id=\"NormalizedQueriesSlowestAverageReport\">Slowest queries (N) <a href=\"#top\" title=\"Back to top\">^</a></h2>";
    output << print_queries(queries.sortedQueries(Queries::mostAverageDuration), top);

    QFile footer("footer.html");
    if (!footer.open(QIODevice::ReadOnly | QIODevice::Text)){
        qout << "Unable to open footer.html" << endl;
        return -5;
    }else{
        output << footer.readAll();
        footer.close();
    }
    qout << "Wrote to file " << args.getString("output-file") << endl;
}
