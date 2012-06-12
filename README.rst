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

::

    qmake
    make
    make test
    sudo make install

Usage
-----

From stdin:

::

    cat /var/log/postgresql/postgresql.log | head -n 100000 | ./pg_query_analyser -i -


From file:

::

    ./pg_query_analyser --input-file=/var/log/postgresql/postgresql.log

For a full build+analyze on Ubuntu:

::

    cd examples
    fab -H <remote-ubuntu-postgres-server> build log_and_analyse

Help
----

::

    # ./pg_query_analyser -h
    Usage: ./pg_query_analyser [flags]
    Options: 
      -h, --help=[false]
      -u, --users=[]
      -v, --verbose=[false]
      -i, --input-file=[/var/log/postgresql/postgresql-9.1-main.log]
      -d, --databases=[]
      -t, --query-types=[SELECT,UPDATE,INSERT,DELETE]
      -o, --output-file=[report.html]
      --top=[20]

