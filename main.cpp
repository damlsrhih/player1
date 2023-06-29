#include "widget.h"
#include <QApplication>

//函数声明, 告诉编译期将会有这么个函数
void a26_test();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();

    a26_test();
    return a.exec();
}
