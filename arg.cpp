#include "arg.h"

Arg::Arg(const char *name, QVariant(*callback)(QString),
        const QVariant default_, const QString help){
    this->_name = name;
    this->setDefault(default_);
    this->setCallback(callback);
    this->setHelp(help);
    this->setRequired(false);
}

Arg::Arg(const char *name, QVariant(*callback)(QString), const QString help){
    this->_name = name;
    this->setCallback(callback);
    this->setHelp(help);
    this->setRequired(true);
}

Arg::Arg(const char *shortname, const char *name,
        QVariant(*callback)(QString), const QVariant default_,
        const QString help){
    this->_name = name;
    this->_shortname = shortname;
    this->setDefault(default_);
    this->setCallback(callback);
    this->setHelp(help);
    this->setRequired(false);
}

Arg::Arg(const char *shortname, const char *name,
        QVariant(*callback)(QString), const QString help){
    this->_name = name;
    this->_shortname = shortname;
    this->setCallback(callback);
    this->setHelp(help);
    this->setRequired(true);
}

Arg::Arg(const Arg &arg) : QObject(){
    _name = arg._name;
    _shortname = arg._shortname;
    _callback = arg._callback;
    _default = arg._default;
    _help = arg._help;
    _required = arg._required;
}

QVariant Arg::writableFile(QString filename){
    return QVariant(filename);
}

QVariant Arg::readableFile(QString filename){
    if(filename == "-" || QFileInfo(filename).isReadable()){
        return QVariant(filename);
    }
    return QVariant();
}

