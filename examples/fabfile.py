from fabric import context_managers, api, contrib
import contextlib
import datetime
import re
import time

ENVIRONMENT_FILES = [
    'fab.env',
    '/etc/fab.env',
    '/usr/etc/fab.env',
    '/usr/local/etc/fab.env',
    '~/.fab.env',
]
AVAILABLE_ENVIRONMENT_FILES = {}


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
    '''Get the env with all variables parsed using Python string formatting

    Example:

    >>> from fabric import api
    >>> api.env.foo = 'The value of Foo!'
    >>> api.env.bar = 'The value of Bar and foo: %(foo)s'
    >>> get_env().bar
    'The value of Bar and foo: The value of Foo!'
    
    The parser does :py:const:`ENV_PARSE_PASSES` over the variables so nested variables
    are supported.

    It is also possible to add host-specific configuration overrides using
    the :py:const:`ENV_OVERRIDES`. Something like this will give you host specific
    support:

    >>> from fabric import api
    >>> ENV_OVERRIDES['host'] = {'my_special_host': {'foo': 'Special Foo!'}}
    >>> api.env.foo = 'The value of Foo!'
    >>> api.env.bar = 'The value of Bar and foo: %(foo)s'
    >>> api.env.host = 'my_special_host'
    >>> get_env().bar
    'The value of Bar and foo: Special Foo!'
    '''
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
    '''Wrap the command with these environment files
    
    By using this context you can add overrides for specific hosts in external
    files or just add global defaults.

    To use the general overrides simply add your settings to one of the files
    in :py:const:`ENVIRONMENT_FILES` and/or add extra files to that file.
    To use the host-specific configuration files the settings must be in one
    of the files mentioned in :py:const:`AVAILABLE_ENVIRONMENT_FILES`.
    '''
    if api.env.host in AVAILABLE_ENVIRONMENT_FILES:
        env_files = AVAILABLE_ENVIRONMENT_FILES[api.env.host]

    else:
        env_files = []
        for env_file in ENVIRONMENT_FILES:
            if contrib.files.exists(env_file):
                env_files.append(env_file)

        AVAILABLE_ENVIRONMENT_FILES[api.env.host] = env_files

    contexts = [context_managers.prefix('source %r' % f) for f in env_files]
    return contextlib.nested(*contexts)

@api.task
def enable_logging():
    '''Enables logging on the Postgres server

    1. :py:func:`Copy <fabric.operations.put>` `pg_query_analyser_log.conf` to the
       remote server
    2. Add `include pg_query_analyser_log.conf ` to the main postgres config file
    3. Reload postgres

    Including the config file is done by
    :py:func:`appending <fabric.contrib.files.append>` the include line to
    the main Postgres config or by :py:func:`uncommenting <uncomment>` the 
    line if already exists.
    '''
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
    '''Comment the given line in the given file using perl
    
    This essentially does the same as the fabric 
    :py:func:`comment <fabric.contrib.files.comment>` method
    but because of weird escaping issues I couldn't get that one to work.
    '''
    api.sudo('perl -pi -e "s/^%s$/# %s/" %s' % (
        line,
        line,
        file,
    ))

def uncomment(file, line):
    '''Uncomment the given line in the given file using perl
    
    This essentially does the same as the fabric 
    :py:func:`uncomment <fabric.contrib.files.uncomment>` method
    but because of weird escaping issues I couldn't get that one to work.
    '''
    api.sudo('perl -pi -e "s/^# %s$/%s/" %s' % (
        line,
        line,
        file,
    ))

@api.task
def disable_logging():
    '''Disable logging on the Postgres server

    1. :py:func:`Comment <comment>` the include for the config file from the
       enable step above
    2. Reload postgres
    '''
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
    '''A waiting task with a ETA indicator

    This task waits :py:attr:`api.env.log_duration` seconds and tells you how much
    time is left.
    '''
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
    '''ubuntu/debian specific command to check if a package is currently
    installed'''
    return bool(api.run('dpkg --get-selections | grep "^%s\s+install$" || true' % package))

def install(package):
    '''Install the package if not installed and returns whether it was
    installed or already existed'''
    if is_installed(package):
        return False
    else:
        api.sudo('apt-get install -y %s' % package)
        return True

def uninstall(package):
    '''Uninstall the package and purge the settings
    
    WARNING: Since this purges the setting this should only be used if this command
    was the command that installed the package'''
    if is_installed(package):
        api.sudo('apt-get purge -y %s' % package)
        return True
    else:
        return False

@api.task
def analyse():
    '''Upload the query analyser, analyse the logs and download the report

    1. Create a temporary directory to do the parsing (default /tmp/postgres)
    2. Check if  libqt4-sql is installed as this is a requirement to run the
       app. if it's not installed, it will be installed automatically.
    3. Upload the pg_query_analyser binary for the platform. automatically
       uploads the version for this ubuntu version with this architecture (if
       you don't have the binary, use the build command)
    4. Run pg_query_analyser over the current logfile
    5. Copy the report to your local machine
    '''
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
    '''Build the application on the remote system and download to the local pc

    1. install `git`, `libqt4-dev` and `qt4-qmake` if needed
    2. git clone the `pg_query_analyser` repository
    3. run `qmake` and `make` in the cloned directory
    4. download the generated binary into
       `pg_query_analyser_<os_version>_<architecture>`
    5. remove the packages from step 1 only if they were installed by this step
    6. remove the temporary directory which was created by the `git clone`
    '''
    installed_git = install('git')
    installed_libqt = install('libqt4-dev')
    installed_qmake = install('qt4-qmake')
    dir_ = 'pg_query_analyser_fab_build'
    api.run('git clone https://github.com/WoLpH/pg_query_analyser.git %s' % dir_)

    with api.cd(dir_):
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

    api.run('rm -rf %s' % dir_)

@api.task
def log_and_analyse():
    '''Do a full log and analyse cycle
    
    1. :py:func:`enable_logging`
    2. :py:func:`wait`
    3. :py:func:`analyse`
    4. :py:func:`disable_logging`
    '''
    enable_logging()
    try:
        wait()
    finally:
        try:
            analyse()
        finally:
            disable_logging()

if __name__ == '__main__':
    import doctest
    doctest.testmod()

