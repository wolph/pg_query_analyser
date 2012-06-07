import contextlib
from fabric import context_managers, api, contrib
import time
import re

environment_files = [
    'fab.env',
    '/etc/fab.env',
    '/usr/etc/fab.env',
    '/usr/local/etc/fab.env',
    '~/.fab.env',
]
available_environment_files = {}


# Special env overrides for a specific env key
# For example, the path on a different host might be different, this works
# by walking through the overrides and filling in that parameter as a key
# so it overrides the current settings with it
#
# NOTE: dictionaries have no set order so the order is _not_ guaranteed
ENV_OVERRIDES = {}
ENV_PARSE_PASSES = 3


api.env.postgres_reload_command = '/etc/init.d/postgresql reload'
api.env.postgres_version = '9.1'
api.env.config_path = '/etc/postgresql/%(postgres_version)s/main'
api.env.config_filename = 'postgresql.conf'
api.env.config_file = '%(config_path)s/%(config_filename)s'
api.env.log_config_filename = 'pg_query_analyser_log.conf'
api.env.log_config_file = '%(config_path)s/%(log_config_filename)s'
api.env.log_include_line = 'include \'%(log_config_filename)s\''
api.env.temp_path = '/mnt/postgres'
api.env.log_duration = 600
api.env.logrotate_command = ('/usr/sbin/logrotate '
    '-f /etc/logrotate.d/postgresql-common')
api.env.log_file = (
    '/var/log/postgresql/postgresql-%(postgres_version)s-main.log')
api.env.pg_query_analyser_file = 'pg_query_analyser-%s-%s'
api.env.pg_query_analyser_command = './pg_query_analyser -i %(log_file)s'
def get_env():
    env = {}
    env.update(api.env)
    for setting, overrides in ENV_OVERRIDES.iteritems():
        for k, v in overrides.iteritems():
            if env[setting] == k:
                env.update(v)

    passes = ENV_PARSE_PASSES
    found = 1
    while passes and found:
        found = 0
        for k, v in env.items():
            if isinstance(v, basestring) and '%(' in v:
                env[k] = v % env
                found += 1

        passes -= 1

    api.env.update(env)
    return api.env


def wrap_environments():
    '''Wrap the command with these environment files'''
    if api.env.host in available_environment_files:
        env_files = available_environment_files[api.env.host]

    else:
        env_files = []
        for env_file in environment_files:
            if contrib.files.exists(env_file):
                env_files.append(env_file)

        available_environment_files[api.env.host] = env_files

    contexts = [context_managers.prefix('source %r' % f) for f in env_files]
    return contextlib.nested(*contexts)

@api.task
def enable_logging():
    env = get_env()
    api.put(
        env.log_config_filename,
        env.config_path,
        use_sudo=True,
    )
    uncomment(
        env.config_file,
        env.log_include_line,
    )
    contrib.files.append(
        env.config_file,
        env.log_include_line,
        use_sudo=True,
    )
    api.sudo(env.postgres_reload_command)
    if env.logrotate_command:
        api.sudo(env.logrotate_command)

def comment(file, line):
    api.sudo('perl -pi -e "s/^%s$/# %s/" %s' % (
        line,
        line,
        file,
    ))

def uncomment(file, line):
    api.sudo('perl -pi -e "s/^# %s$/%s/" %s' % (
        line,
        line,
        file,
    ))

@api.task
def disable_logging():
    env = get_env()
    comment(
        env.config_file,
        re.escape(env.log_include_line),
    )
    api.sudo(env.postgres_reload_command)
    if env.logrotate_command:
        api.sudo(env.logrotate_command)

@api.task
def wait():
    env = get_env()
    start_time = time.time()
    duration = time.time() - start_time
    while duration < env.log_duration:
        duration = time.time() - start_time
        eta = env.log_duration - duration
        
        seconds = eta % 60
        minutes = int(eta / 60) % 60
        hours = int(eta / 3600)
        
        eta_parts = []
        if hours:
            eta_parts.append('%d hours' % hours)
        if minutes:
            eta_parts.append('%d minutes' % minutes)
        if seconds:
            eta_parts.append('%d seconds' % seconds)

        print 'Current eta:',
        if eta_parts[2:]:
            print '%s, %s' % (
                ', '.join(eta_parts[:-2]),
                ' and '.join(eta_parts[-2:]),
            )
        elif eta_parts[1:]:
            print ' and '.join(eta_parts[-2:])
        elif eta_parts:
            print eta_parts[0]
        else:
            print 'now'

        time.sleep(1)

def install(package):
    if not api.run('dpkg --get-selections | grep %s || true' % package):
        api.sudo('apt-get install %s' % package)

@api.task
def analyze():
    env = get_env()
    api.sudo('mkdir -p %s' % env.temp_path)
    api.sudo('chown -R %s %s' % (env.user, env.temp_path))

    install('libqt4-dev')
    install('qt4-qmake')

    architecture = api.run('uname -m')
    os_version = api.run(
        'grep -E \'^DISTRIB_RELEASE=\' /etc/lsb-release '
        '| awk -F \'=\' \'{print $2}\''
    )
    pg_query_analyser = api.env.pg_query_analyser_file % (
        os_version,
        architecture,
    )

    api.put(pg_query_analyser, 'pg_query_analyser', mode=0755)
    api.run(env.pg_query_analyser_command)

@api.task
def analyze_db():
    env = get_env()
    analyze()
    return
    enable_logging()
    try:
        wait()
    finally:
        try:
            analyze()
        finally:
            disable_logging()

    #database = database or api.env.default_database
    #info = api.host_info[api.env.host_string]
    #db_name = info.tags.get('Name')
    #api.sudo('perl -pi -e "s/log_min_duration_statement = .*/log_min_duration_statement = 0/" /etc/postgresql/9.*/main/postgresql.conf')
    #api.sudo('/etc/init.d/postgresql reload')
    #time.sleep(30)
    #api.sudo('perl -pi -e "s/log_min_duration_statement = .*/log_min_duration_statement = 500/" /etc/postgresql/9.*/main/postgresql.conf')
    #api.sudo('/etc/init.d/postgresql reload')
    #api.run('tail -n 100000 /var/log/postgresql/postgresql-9.*-main.log > /tmp/pgfouine.txt')
    #api.run('gzip -f /tmp/pgfouine.txt')
    #api.get('/tmp/pgfouine.txt.gz', local_path = '/tmp/latest-pgfouine.txt.gz')
    #api.local('gunzip -f /tmp/latest-pgfouine.txt.gz')
    #now = int(time.time())
    #api.local('~/src/pgfouine/pgfouine.php -logtype stderr -file /tmp/latest-pgfouine.txt -quiet > /tmp/pgfouine-%s-%d.html 2>1' % (db_name, now))
    #api.local('open /tmp/pgfouine-%s-%d.html' % (db_name, now) )

