#include "args.h"

bool Args::parse(int argc, char **argv){
    /* Simple class to parse some commandline arguments */

    QTextStream qerr(stderr, QIODevice::WriteOnly);
    /* to accept parameters like --foo-bar=spam-eggs */
    QRegExp key_value_re("--([^=]+)=(.*)");
    /* to accept parameters like --foo-bar */
    QRegExp key_re("--([^=]+)");
    QString tmp;
    QHash<QString, QString> args;
    /* Using argc/argv so all parameters work, not just the ones not filtered
     out by Qt */
    program = QString(argv[0]);

    for(int i=1; i<argc; i++){
        tmp = QString(argv[i]);
        /* Called help, exiting to display help */
        if(tmp == "-h"){
            return false;
        }

        if(key_value_re.exactMatch(tmp)){
            args.insert(key_value_re.cap(1), key_value_re.cap(2));
        }else if(key_re.exactMatch(tmp)){
            tmp = key_re.cap(1);

            /* Called help, exiting to display help */
            if(tmp == "help"){
                return false;
            }

            if(i + 1 == argc || key_re.exactMatch(QString(argv[i+1])) || key_value_re.exactMatch(QString(argv[i+1]))){
                args.insert(tmp, "");
            }else{
                args.insert(tmp, argv[i+1]);
                i++;
            }
        }
    }

    QHashIterator<QString, Arg*> i(this->args);
    while(i.hasNext()){
        i.next();
        Arg arg = *i.value();
        if(args.contains(arg.getName())){
            insert(arg.getName(), arg.callback(args.value(arg.getName())));
        }else if(arg.getRequired()){
            qerr << "Argument '" << i.key() << "' is required and not given." << endl;
            return false;
        }else{
            insert(arg.getName(), arg.getDefault());
        }
    }
    return true;
}

void Args::add(Arg *arg){
    args.insert(arg->getName(), arg);
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

int Args::getFile(QFile *filePtr, QString key, QFlags<QIODevice::OpenModeFlag> flags){
    QString filename = getString(key);
    QFile file(filePtr);
    QTextStream qerr(stderr, QIODevice::WriteOnly);

    if(filename == "-"){
        if(flags & QIODevice::ReadOnly){
            qerr << "Reading from stdin" << endl;
            
            if(!file.open(stdin, flags)){
                qerr << "Unable to read stdin, exiting" << endl;
                return -2;
            }
        }else{
            qerr << "Writing to stdout" << endl;
            
            if(!file.open(stdout, flags)){
                qerr << "Unable to write to stdout, exiting" << endl;
                return -3;
            }
        }        
    }else{
        qerr << "Opening " << filename << endl;

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
            qerr << "Unable to open '" << filename << "' exiting" << endl;
            return -4;
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
        qerr << QString("%1").arg("--" + i.key(), 20);
        if(i.value()->getRequired()){
            qerr << QString("=<%1>").arg(i.key().toUpper(), -30);
        }else{
            qerr << QString("=%1").arg(i.value()->getDefault().toString(), -30);
        }
        if(i.value()->getHelp().length()){
            qerr << " -- " << i.value()->getHelp();
        }
        qerr << endl;
    }
}
