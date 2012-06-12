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


Explanation of the `fabfile.py`:


enable logging
Copy https://github.com/WoLpH/pg_query_analyser/blob/master/examples/pg_query_analyser_log.conf to the remote server
Include that config file in Postgres
Reload postgres
wait
wait for log_duration time, default is currently 60 seconds
ideally we want to parse at least 30 minutes since 60 seconds gives a really bad overview, but that's not possible till we can change the logging location temporarily.

analyse
Create a temporary directory to do the parsing (default /tmp/postgres)
Check if  libqt4-sql is installed as this is a requirement to run the app. if it's not installed, it will be installed automatically.
Upload the pg_query_analyser binary for the platform. automatically uploads the version for this ubuntu version with this architecture (if you don't have the binary, use the build command)
Run pg_query_analyser over the current logfile
Copy the report to your local machine
disable logging
Comment the include for the config file from the enable step above
Reload postgres
build
If you don't have the binary available you can use the build step to automatically build the binary.
It will access the given remote server and execute the following steps:
install the required packages if needed (git, qt4-make, libqt4-dev)
git clone the app
build the app
download the binary to pg_query_analyser_<ubuntu version>_<architecture>
uninstall the packages if they were installed by this command but leaves them be if they were already available before running the command


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

