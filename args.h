#ifndef ARGS_H
#define ARGS_H

#include <QHash>
#include <QVariant>
#include <QFile>
#include <QStringList>
#include <arg.h>

class Args : public QHash<QString, QVariant>{
private:
    QHash<QString, Arg*> args;
    QString program;

public:
    bool parse(int argc, char **argv);
    void add(Arg *arg);

    QString getString(QString key);
    QStringList getStringList(QString key, QString separator=",");
    int getInt(QString key);
    bool getBool(QString key);

    void help();
};

#endif // ARGS_H
