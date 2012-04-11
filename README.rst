=====================================================
pg_query_analyser -- PostgreSQL Slow Query Log parser
=====================================================

Overview
--------

pg_query_analyser is a C++ clone of the PgFouine log analyser.

Processing logs with millions of lines only takes a few minutes with this
parser while PgFouine chokes long before that.

Install
-------

..
    qmake
    make

Usage
-----

From stdin:

..
    cat /var/log/postgresql/postgresql.log | head -n 100000 | ./pg_query_analyser --from_stdin=true

From file:

..
    ./pg_query_analyser --input_file=/var/log/postgresql/postgresql.log

Help
----

..
    # ./pg_query_analyser --help                                                                                                                                                                                                                                                           /home/rick/pg_query_analyser
    Usage: ./pg_query_analyser [flags]
    Options: 
            --input_file=/var/log/postgresql/postgresql-8.4-main.log
            --users=                              
            --verbose=false                         
            --from_stdin=false                         
            --databases=                              
            --output_file=/usr/local/www/phpPgAdmin/report.html   

