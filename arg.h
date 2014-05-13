#ifndef ARG_H
#define ARG_H

#include <QObject>
#include <QVariant>
#include <QString>
#include <QFileInfo>

class Arg : public QObject{
public:
    Arg(const char *name, QVariant(*callback)(QString), QVariant default_, QString help="");
    Arg(const char *name, QVariant(*callback)(QString), QString help="");
    Arg(const char *shortname, const char *name, QVariant(*callback)(QString), QVariant default_, QString help="");
    Arg(const char *shortname, const char *name, QVariant(*callback)(QString), QString help="");
    Arg(const Arg &arg);

    QString getName() const {return _name;}
    QString getShortname() const {return _shortname;}

    void setCallback(QVariant(*callback)(QString)){_callback = callback;}
    QVariant callback(QString parameter){return _callback(parameter);}

    void setDefault(const QVariant default_){_default = default_;}
    QVariant getDefault() const {return _default;}

    void setRequired(const bool required){_required = required;}
    bool getRequired() const {return _required;}

    QString getHelp() const {return _help;}
    void setHelp(const QString help){_help = help;}
    void setHelp(const char *help){setHelp(QString(help));}

    static QVariant setTrue(QString){return QVariant(true);}
    static QVariant setFalse(QString){return QVariant(false);}
    static QVariant readableFile(QString);
    static QVariant writableFile(QString);
    static QVariant toString(QString input){return QVariant(input);}
    static QVariant toInt(QString input){return QVariant(input.toInt());}

private:
    QString _name;
    QString _shortname;
    QVariant(*_callback)(QString);
    QVariant _default;
    QString _help;
    bool _required;
};

#endif // ARG_H
