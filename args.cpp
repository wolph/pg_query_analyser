#include <QTextStream>
#include <QRegExp>
#include "args.h"

bool Args::parse(int argc, char **argv){
    QTextStream cerr(stderr, QIODevice::WriteOnly);
    QRegExp key_value_re("--([^=]+)=(.*)");
    QRegExp key_re("--([^=]+)");
    QString tmp;
    QHash<QString, QString> args;
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
            cerr << "Argument '" << i.key() << "' is required and not given." << endl;
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

void Args::help(){
    QTextStream cerr(stderr, QIODevice::WriteOnly);
    cerr << "Usage: " << program << " [flags]" << endl;
    cerr << "Options: " << endl;

    QHashIterator<QString, Arg*> i(this->args);
    while(i.hasNext()){
        i.next();
        cerr << QString("%1").arg("--" + i.key(), 20);
        if(i.value()->getRequired()){
            cerr << QString("=<%1>").arg(i.key().toUpper(), -30);
        }else{
            cerr << QString("=%1").arg(i.value()->getDefault().toString(), -30);
        }
        if(i.value()->getHelp().length()){
            cerr << " -- " << i.value()->getHelp();
        }
        cerr << endl;
    }
}
