#include <main.h>

QTextStream cout(stdout, QIODevice::WriteOnly);
QTextStream cerr(stderr, QIODevice::WriteOnly);
QTextStream cin(stdin, QIODevice::ReadOnly);

int min(int a, int b){
    return a < b ? a : b;
}

int example = 0;

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
          "<td><pre>%8</pre></td></tr>"
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
            .arg(q->getStatement()) \
            .arg(example);

        QStringList examples = q->getExamples();
        QList<uint> durations = q->getDurations();
        for(int j=0; j<examples.count(); j++){
            output << QString("<div class=\"example%1 sql\">%2 | %3</div>") \
                .arg(j % 2) \
                .arg(durations.at(j)) \
                .arg(examples.at(j));
        }

        output << "</div></td></tr>";
        example++;
    }
    output << "</table>";

    return output_string;
}

int main(int argc, char **argv){
    Args args;
    args.add(new Arg("verbose", Arg::setTrue, QVariant(false)));
    args.add(new Arg("from_stdin", Arg::setTrue, QVariant(false)));
    args.add(new Arg("input_file", Arg::readableFile, QVariant("/var/log/postgresql/postgresql-8.4-main.log")));
    //args.add(new Arg("input_file", Arg::readableFile, QVariant("/var/log/postgresql/test.log")));
    args.add(new Arg("output_file", Arg::writableFile, QVariant("/usr/local/www/phpPgAdmin/report.html")));
    args.add(new Arg("users", Arg::toString, QVariant()));
    args.add(new Arg("databases", Arg::toString, QVariant()));
    args.add(new Arg("top", Arg::toInt, QVariant(20)));

    if(!args.parse(argc, argv)){
        args.help();
        return -1;
    }

    QStringList users = args.getStringList("users");
    QStringList databases = args.getStringList("databases");

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
    QFile input_file(args.getString("input_file"));

    /* display the top N queries */
    int top = args.getInt("top");

    if(args.getBool("from_stdin")){
        cout << "Reading from stdin" << endl;
        
        if (!input_file.open(stdin, QIODevice::ReadOnly | QIODevice::Text)){
            cerr << "Unable to read file, exiting" << endl;
            return -2;
        }
        
    }else{
        cout << "Opening " << args.getString("input_file") << endl;

        if (!input_file.open(QIODevice::ReadOnly | QIODevice::Text)){
            cerr << "Unable to read file, exiting" << endl;
            return -2;
        }
    }

    QFile output_file(args.getString("output_file"));
    if (!output_file.open(QIODevice::WriteOnly | QIODevice::Text)){
        cerr << "Unable to open output file " << args.getString("output_file") << endl;
        return -3;
    }
    QTextStream output(&output_file);

    Queries queries;
    old_query_id = -1;
    old_line_id = -0;
    statement = "";
    duration = 0;

    QTime timer;
    timer.start();

    uint lines = 0;
    while (!input_file.atEnd()) {
        line = input_file.readLine(4096);
        lines++;
        if(lines % 1000 == 0){
            cout << "Read " << lines << " lines." << endl;
            cout.flush();
        }

        // Oct 15 13:00:04 db1 postgres[91094]: [123-1] user=pytm_alchemy,db=pytm LOG:  unexpected EOF on client connection
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
            statement.replace(">", "&gt;");
            statement.replace("<", "&lt;");
            QString hashStatement = Query::normalize(statement);
            statement = Query::format(statement);
            //if((
            //        hashStatement.startsWith("INSERT") ||
            //        hashStatement.startsWith("DELETE") ||
            //        hashStatement.startsWith("UPDATE")
            //    )
            if((
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
        cerr << "Unable to open header.html" << endl;
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
        cerr << "Unable to open footer.html" << endl;
        return -5;
    }else{
        output << footer.readAll();
        footer.close();
    }
    cerr << "Wrote to file " << args.getString("output_file") << endl;
}
