from fabric import context_managers, api, contrib
import contextlib
import datetime
import re
import time

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
api.env.log_duration = 60
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
        eta = max(env.log_duration - duration, 0)
        
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

def is_installed(package):
    return bool(api.run('dpkg --get-selections | grep "^%s\s+install$" || true' % package))

def install(package):
    if is_installed(package):
        return False
    else:
        api.sudo('apt-get install -y %s' % package)
        return True

def uninstall(package):
    if is_installed(package):
        api.sudo('apt-get purge -y %s' % package)
        return True
    else:
        return False

@api.task
def analyse():
    env = get_env()
    api.sudo('mkdir -p %s' % env.temp_path)
    api.sudo('chown -R %s %s' % (env.user, env.temp_path))
    with api.cd(env.temp_path):
        install('libqt4-sql')

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
        api.sudo(env.pg_query_analyser_command)
        api.get('report.html', 'report_%s_%s.html' % (
            env.host,
            datetime.datetime.now().strftime('%Y%m%d%H%M%S'),
        ))

@api.task
def build():
    installed_git = install('git')
    installed_libqt = install('libqt4-dev')
    installed_qmake = install('qt4-qmake')
    dir = 'pg_query_analyser_fab_build'
    api.run('git clone https://github.com/WoLpH/pg_query_analyser.git %s' % dir)

    with api.cd(dir):
        api.run('qmake')
        api.run('make')

        architecture = api.run('uname -m')
        os_version = api.run(
            'grep -E \'^DISTRIB_RELEASE=\' /etc/lsb-release '
            '| awk -F \'=\' \'{print $2}\''
        )
        pg_query_analyser = api.env.pg_query_analyser_file % (
            os_version,
            architecture,
        )
        api.get('pg_query_analyser', pg_query_analyser)

    autoremove = False
    if installed_git:
        uninstall('git')
        autoremove = True
    if installed_libqt:
        uninstall('libqt4-dev')
        autoremove = True
    if installed_qmake:
        uninstall('qt4-qmake')
        autoremove = True

    if autoremove:
        api.sudo('apt-get autoremove -y')

    api.run('rm -rf %s' % dir)

@api.task
def log_and_analyse():
    enable_logging()
    try:
        wait()
    finally:
        try:
            analyse()
        finally:
            disable_logging()

