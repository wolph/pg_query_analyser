#include "args.h"

bool Args::parse(int argc, char **argv){
    /* Simple class to parse some commandline arguments */
    QTextStream qerr(stderr, QIODevice::WriteOnly);
    QHash<QString, QString> args;

    /* to accept parameters like -f=spam-eggs */
    QRegExp short_key_value_re("-([^=-])=(.+)");
    /* to accept parameters like -f */
    QRegExp short_key_re("-([^=-]+)");
    /* to accept parameters like --foo-bar=spam-eggs */
    QRegExp key_value_re("--([^=]+)=(.+)");
    /* to accept parameters like --foo-bar */
    QRegExp key_re("--([^=]+)");
    /* to accept values for space separated parameters */
    QRegExp value_re("([^-]+.*|.)");

    /* Using argc/argv so all parameters work, not just the ones not filtered
     out by Qt */
    program = QString(argv[0]);

    for(int i=1; i<argc; i++){
        int j;

        QString key;
        QString value;
        QString tmp = QString(argv[i]);

        if(short_key_value_re.exactMatch(tmp)){
            key = short_key_value_re.cap(1);
            value = short_key_value_re.cap(2);
            if(key.length() > 1){
                qerr << "Can't have multiple short arguments with values." \
                    << endl;
                qerr << "Did you forget a '-'? Try --" << key << "=" \
                    << value << " instead" << endl;
                return false;
            }
        }else if(short_key_re.exactMatch(tmp)){
            key = short_key_re.cap(1);
            if(short_key_re.cap(1).length() != 1){
                j = short_key_re.cap(1).length();
                while(j--){
                    args.insert(QString(key[j]), value);
                }
                continue;
            }
        }else if(key_value_re.exactMatch(tmp)){
            key = key_value_re.cap(1);
            value = key_value_re.cap(2);

        }else if(key_re.exactMatch(tmp)){
            key = key_re.cap(1);
        }

        if(i+1 < argc){
            tmp = QString(argv[i+1]);
            if(value == NULL && value_re.exactMatch(tmp)){
                i++;
                value = value_re.cap(1);
            }
        }
        args.insert(key, value);
    }

    QHashIterator<QString, Arg*> j(this->args);
    while(j.hasNext()){
        j.next();
        Arg arg = *j.value();

        if(args.contains(arg.getName())){
            /* Called help, exiting to display help */
            if(arg.getName() == "help"){
                return false;
            }
            insert(arg.getName(), arg.callback(args.take(arg.getName())));
        }else if(args.contains(arg.getShortname())){
            /* Called help, exiting to display help */
            if(arg.getName() == "help"){
                return false;
            }
            insert(arg.getName(),
                arg.callback(args.take(arg.getShortname())));
        }else if(arg.getRequired()){
            qerr << "Argument '" << j.key() \
                << "' is required and not given." << endl;
            return false;
        }else{
            insert(arg.getName(), arg.getDefault());
        }
    }

    if(!args.empty()){
        qerr << "The following unsupported arguments were found:" << endl;
        QHashIterator<QString, QString> j(args);
        while(j.hasNext()){
            j.next();
            if(j.key().length() == 1){
                qerr << "\t-";
            }else{
                qerr << "\t--";
            }
            qerr << j.key();
            if(j.value() != ""){
                qerr << " with value: " << j.value();
            }
            qerr << endl;
        }
        return false;
    }
    return true;
}

void Args::add(Arg *arg){
    args.insert(arg->getName(), arg);
    if(arg->getShortname() != "")
        shortArgs.insert(arg->getShortname(), arg);
}

QString Args::getString(QString key){
    return value(key).toString();
}

QStringList Args::getStringList(QString key, QString separator){
    if(getString(key).trimmed().length()){
        return getString(key).split(separator);
    }else{
        return QStringList();
    }
}

int Args::getInt(QString key){
    return value(key).toInt();
}

bool Args::getBool(QString key){
    return value(key).toBool();
}

int Args::getFile(QFile *file, QString key,
        QFlags<QIODevice::OpenModeFlag> flags){
    QTextStream qerr(stderr, QIODevice::WriteOnly);
    QString filename = getString(key);

    if(filename == "-"){
        if(flags & QIODevice::ReadOnly){
            qerr << "Reading from stdin" << endl;
            
            if(!file->open(stdin, flags)){
                qerr << "Unable to read stdin, exiting" << endl;
                return -2;
            }
        }else{
            qerr << "Writing to stdout" << endl;
            
            if(!file->open(stdout, flags)){
                qerr << "Unable to write to stdout, exiting" << endl;
                return -3;
            }
        }
    }else{
        file->setFileName(filename);
        if(flags & QIODevice::ReadOnly){
            qerr << "Reading from " << filename << endl;
            if (!file->open(flags)){
                qerr << "Unable to open '" << filename << "', exiting" << endl;
                return -4;
            }
        }else{
            qerr << "Writing to " << filename << endl;
            if (!file->open(flags)){
                qerr << "Unable to open '" << filename << "', exiting" << endl;
                return -4;
            }
        }
    }
    return 0;
}

void Args::help(){
    QTextStream qerr(stderr, QIODevice::WriteOnly);
    qerr << "Usage: " << program << " [flags]" << endl;
    qerr << "Options: " << endl;

    QHashIterator<QString, Arg*> i(this->args);
    while(i.hasNext()){
        i.next();
        Arg arg = *i.value();

        /* print the short argument */
        if(arg.getShortname() == ""){
            qerr << QString("%1  ").arg("", 4);
        }else{
            qerr << QString("%1, ").arg("-" + arg.getShortname(), 4);
        }

        /* print the long argument and the default value */
        if(i.value()->getRequired()){
            qerr << QString("%1=<%2>").arg("--" + i.key(), i.key().toUpper());
        }else{
            qerr << QString("%1=[%2]").arg("--" + i.key(),
                arg.getDefault().toString());
        }

        /* print the help */
        if(i.value()->getHelp().length()){
            qerr << " -- " << i.value()->getHelp();
        }
        qerr << endl;
    }
}

