#ifndef ARGS_H
#define ARGS_H

#include <QHash>
#include <QVariant>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QRegExp>
#include "arg.hpp"

class Args : public QHash<QString, QVariant>{
private:
    QHash<QString, Arg*> args;
    QHash<QString, Arg*> shortArgs;
    QString program;

public:
    bool parse(int argc, char **argv);
    void add(Arg *arg);

    QString getString(QString key);
    int getFile(QFile *file, QString key,
        QFlags<QIODevice::OpenModeFlag> flags);
    QStringList getStringList(QString key, QString separator=",");
    int getInt(QString key);
    bool getBool(QString key);

    void help();
};

#endif // ARGS_H
